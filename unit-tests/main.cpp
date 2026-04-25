/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Nihilai Collective Corp
 * https://github.com/nihilai-collective/void-containers
 * unit-tests/main.cpp
 */

#include <void-containers>
#include <rt-ut>
#include <bit>
#include <cstdint>
#include <iostream>
#include <string>

int32_t run_sonnet_map_tests();
int32_t run_sonnet_set_tests();

namespace tests {

	using namespace void_containers::internal;

	struct cpu_alignment_is_power_of_two {
		static bool impl() {
			return std::has_single_bit(cpu_properties::get_value(cpu_property_types::alignment));
		}
	};

	struct bit_ops_match_definitions {
		static bool impl() {
			return lzcnt(uint32_t{ 1 }) == 31U && tzcnt(uint32_t{ 8 }) == 3U && tzcnt(uint16_t{ 0 }) == 16U && tzcnt(uint64_t{ 1ULL << 40 }) == 40ULL &&
				blsr(uint16_t{ 0b1100 }) == uint16_t{ 0b1000 } && blsr(uint64_t{ 0 }) == 0ULL;
		}
	};

	struct compare_finds_a_difference_at_every_length {
		static bool impl() {
			char lhs[64]{};
			char rhs[64]{};
			for (uint64_t length = 1; length <= 63; ++length) {
				for (uint64_t position = 0; position < length; ++position) {
					rhs[position] = 1;
					if (comparison::compare(lhs, rhs, length)) {
						return false;
					}
					rhs[position] = 0;
				}
				if (!comparison::compare(lhs, rhs, length)) {
					return false;
				}
			}
			return true;
		}
	};

	struct sonnet_map_handles_integer_keys {
		static bool impl() {
			void_containers::sonnet_map<uint32_t, uint32_t> map{};
			for (uint32_t key = 0; key < 1000; ++key) {
				map[key] = key * 2U;
			}
			for (uint32_t key = 0; key < 500; ++key) {
				map.erase(key);
			}
			void_containers::sonnet_map<uint32_t, uint32_t> copy{};
			copy = map;
			for (uint32_t key = 1000; key < 3000; ++key) {
				copy.insert(key, key);
			}
			uint64_t visited{};
			for (auto& entry: copy) {
				visited += entry.first >= 500U ? 1ULL : 0ULL;
			}
			return map.size() == 500 && copy.size() == 2500 && visited == 2500 && !copy.contains(1U) && copy[999U] == 1998U;
		}
	};

	struct sonnet_map_handles_string_keys {
		static bool impl() {
			void_containers::sonnet_map<std::string, uint32_t> map{};
			for (uint32_t key = 0; key < 200; ++key) {
				map.insert(std::string(key % 70U, 'x') + std::to_string(key), key);
			}
			const std::string present{ std::string(5, 'x') + "5" };
			const std::string absent{ "not there" };
			return map.size() == 200 && map[present] == 5U && !map.contains(absent);
		}
	};

	template<typename simd_class> struct cmp_eq_reports_every_lane {
		static bool impl() {
			using mask_type = typename simd_class::mask_type;
			alignas(64) uint8_t bytes[64 + 1]{};
			for (uint64_t lane = 0; lane < simd_class::byte_size; ++lane) {
				bytes[lane]		= 0x7F;
				bytes[lane + 1] = 0;
				const auto loaded{ load_values<simd_class>(bytes) };
				if (op_cmp_eq<simd_class>(op_set1<simd_class>(uint8_t{ 0x7F }), loaded) != static_cast<mask_type>(static_cast<mask_type>(1) << lane)) {
					return false;
				}
				bytes[lane] = 0;
			}
			return true;
		}
	};

	template<typename simd_class> struct cmp_eq_matches_high_bit_bytes {
		static bool impl() {
			const auto empty{ op_set1<simd_class>(uint8_t{ 0x80 }) };
			return op_cmp_eq<simd_class>(empty, empty) == static_cast<typename simd_class::mask_type>(~typename simd_class::mask_type{});
		}
	};

	template<typename simd_class> struct unaligned_load_keeps_byte_order {
		static bool impl() {
			alignas(64) uint8_t bytes[64 + 1]{};
			bytes[1] = 0x55;
			return op_cmp_eq<simd_class>(load_values_u<simd_class>(bytes + 1), op_set1<simd_class>(uint8_t{ 0x55 })) == typename simd_class::mask_type{ 1 };
		}
	};

	template<typename simd_class> struct test_sees_any_single_lane {
		static bool impl() {
			alignas(64) uint8_t bytes[64]{};
			if (op_test<simd_class>(load_values<simd_class>(bytes))) {
				return false;
			}
			for (uint64_t lane = 0; lane < simd_class::byte_size; ++lane) {
				bytes[lane] = 1;
				if (!op_test<simd_class>(load_values<simd_class>(bytes))) {
					return false;
				}
				bytes[lane] = 0;
			}
			return true;
		}
	};

	template<typename simd_class> struct xor_and_or_are_bitwise {
		static bool impl() {
			const auto low{ op_set1<simd_class>(uint8_t{ 0x0F }) };
			const auto high{ op_set1<simd_class>(uint8_t{ 0xF0 }) };
			const auto all{ op_set1<simd_class>(uint8_t{ 0xFF }) };
			const auto full_mask{ static_cast<typename simd_class::mask_type>(~typename simd_class::mask_type{}) };
			return !op_test<simd_class>(op_xor<simd_class>(low, low)) && op_cmp_eq<simd_class>(op_or<simd_class>(low, high), all) == full_mask &&
				op_cmp_eq<simd_class>(op_xor<simd_class>(low, all), high) == full_mask;
		}
	};

}

int32_t main() {
	try {
		rt_ut::unit_test<"cpu_alignment_is_power_of_two">::run(&tests::cpu_alignment_is_power_of_two::impl);
		rt_ut::unit_test<"bit_ops_match_definitions">::run(&tests::bit_ops_match_definitions::impl);

		using simd_128 = tests::simd_int_128_t;
		rt_ut::unit_test<"cmp_eq_reports_every_lane_128">::run(&tests::cmp_eq_reports_every_lane<simd_128>::impl);
		rt_ut::unit_test<"cmp_eq_matches_high_bit_bytes_128">::run(&tests::cmp_eq_matches_high_bit_bytes<simd_128>::impl);
		rt_ut::unit_test<"unaligned_load_keeps_byte_order_128">::run(&tests::unaligned_load_keeps_byte_order<simd_128>::impl);
		rt_ut::unit_test<"test_sees_any_single_lane_128">::run(&tests::test_sees_any_single_lane<simd_128>::impl);
		rt_ut::unit_test<"xor_and_or_are_bitwise_128">::run(&tests::xor_and_or_are_bitwise<simd_128>::impl);

#if VC_SIMD_HAS_256
		using simd_256 = tests::simd_int_256_t;
		rt_ut::unit_test<"cmp_eq_reports_every_lane_256">::run(&tests::cmp_eq_reports_every_lane<simd_256>::impl);
		rt_ut::unit_test<"cmp_eq_matches_high_bit_bytes_256">::run(&tests::cmp_eq_matches_high_bit_bytes<simd_256>::impl);
		rt_ut::unit_test<"unaligned_load_keeps_byte_order_256">::run(&tests::unaligned_load_keeps_byte_order<simd_256>::impl);
		rt_ut::unit_test<"test_sees_any_single_lane_256">::run(&tests::test_sees_any_single_lane<simd_256>::impl);
		rt_ut::unit_test<"xor_and_or_are_bitwise_256">::run(&tests::xor_and_or_are_bitwise<simd_256>::impl);
#endif

#if VC_SIMD_HAS_512
		using simd_512 = tests::simd_int_512_t;
		rt_ut::unit_test<"cmp_eq_reports_every_lane_512">::run(&tests::cmp_eq_reports_every_lane<simd_512>::impl);
		rt_ut::unit_test<"cmp_eq_matches_high_bit_bytes_512">::run(&tests::cmp_eq_matches_high_bit_bytes<simd_512>::impl);
		rt_ut::unit_test<"unaligned_load_keeps_byte_order_512">::run(&tests::unaligned_load_keeps_byte_order<simd_512>::impl);
		rt_ut::unit_test<"test_sees_any_single_lane_512">::run(&tests::test_sees_any_single_lane<simd_512>::impl);
		rt_ut::unit_test<"xor_and_or_are_bitwise_512">::run(&tests::xor_and_or_are_bitwise<simd_512>::impl);
#endif

		rt_ut::unit_test<"compare_finds_a_difference_at_every_length">::run(&tests::compare_finds_a_difference_at_every_length::impl);
		rt_ut::unit_test<"sonnet_map_handles_integer_keys">::run(&tests::sonnet_map_handles_integer_keys::impl);
		rt_ut::unit_test<"sonnet_map_handles_string_keys">::run(&tests::sonnet_map_handles_string_keys::impl);

		const int32_t map_result{ run_sonnet_map_tests() };
		const int32_t set_result{ run_sonnet_set_tests() };
		return map_result != 0 ? map_result : set_result;
	} catch (const rt_ut::rt_ut_exception& e) {
		std::cout << "Error: " << e.what() << std::endl;
		return 1;
	}
}
