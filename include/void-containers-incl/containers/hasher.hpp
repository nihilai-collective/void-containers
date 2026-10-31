/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Nihilai Collective Corp
 * https://github.com/nihilai-collective/void-containers
 * include/void-containers-incl/containers/hasher.hpp
 */
#pragma once

#include <void-containers-incl/config.hpp>
#include <void-containers-incl/containers/concepts.hpp>

namespace void_containers {

	template<typename value_type, uint64_t seed_new = 14695981039346656037ULL> struct rt_key_hasher {};

	VC_INLINE uint64_t load_uint64(const char* value) noexcept {
		uint64_t return_value;
		std::memcpy(&return_value, value, sizeof(uint64_t));
		return return_value;
	}

	VC_INLINE constexpr uint64_t fmix64(uint64_t value) noexcept {
		value ^= value >> 33;
		value *= 0xff51afd7ed558ccdULL;
		value ^= value >> 33;
		value *= 0xc4ceb9fe1a85ec53ULL;
		value ^= value >> 33;
		return value;
	}

	inline constexpr uint64_t mix64_multiplier{ 0x9DDFEA08EB382D69ULL };

	VC_INLINE uint64_t mix64(const uint64_t value, const uint64_t multiplier) noexcept {
#if VC_COMPILER_MSVC && VC_ARCH_X64
		uint64_t high_part;
		const uint64_t low_part{ _umul128(value, multiplier, &high_part) };
		return low_part ^ high_part;
#elif VC_COMPILER_MSVC
		return (value * multiplier) ^ __umulh(value, multiplier);
#else
		const __uint128_t product{ static_cast<__uint128_t>(value) * static_cast<__uint128_t>(multiplier) };
		return static_cast<uint64_t>(product) ^ static_cast<uint64_t>(product >> 64U);
#endif
	}

	template<string_types key_type_newer, uint64_t seed_new> struct rt_key_hasher<key_type_newer, seed_new> {
		static constexpr uint64_t seed{ seed_new };

		template<string_types key_type> VC_INLINE uint64_t operator()(const key_type& value) const noexcept {
			static constexpr uint64_t c1{ 0x87c37b91114253d5ULL };
			static constexpr uint64_t c2{ 0x4cf5ad432745937fULL };

			const char* data{ std::bit_cast<const char*>(value.data()) };
			const uint64_t length{ static_cast<uint64_t>(value.size()) };

			uint64_t h1{ seed };
			uint64_t h2{ seed };

			const uint64_t block_count{ length / 16ULL };
			for (uint64_t index = 0; index < block_count; ++index) {
				uint64_t k1{ load_uint64(data + index * 16ULL) };
				uint64_t k2{ load_uint64(data + index * 16ULL + 8ULL) };

				k1 *= c1;
				k1 = std::rotl(k1, 31);
				k1 *= c2;
				h1 ^= k1;
				h1 = std::rotl(h1, 27);
				h1 += h2;
				h1 = h1 * 5ULL + 0x52dce729ULL;

				k2 *= c2;
				k2 = std::rotl(k2, 33);
				k2 *= c1;
				h2 ^= k2;
				h2 = std::rotl(h2, 31);
				h2 += h1;
				h2 = h2 * 5ULL + 0x38495ab5ULL;
			}

			const char* tail{ data + block_count * 16ULL };
			const uint64_t tail_length{ length & 15ULL };
			uint64_t k1{};
			uint64_t k2{};

			if (tail_length > 8ULL) {
				for (uint64_t index = tail_length; index > 8ULL; --index) {
					k2 ^= static_cast<uint64_t>(static_cast<unsigned char>(tail[index - 1ULL])) << ((index - 9ULL) * 8ULL);
				}
				k2 *= c2;
				k2 = std::rotl(k2, 33);
				k2 *= c1;
				h2 ^= k2;
			}
			if (tail_length > 0ULL) {
				for (uint64_t index = tail_length < 8ULL ? tail_length : 8ULL; index > 0ULL; --index) {
					k1 ^= static_cast<uint64_t>(static_cast<unsigned char>(tail[index - 1ULL])) << ((index - 1ULL) * 8ULL);
				}
				k1 *= c1;
				k1 = std::rotl(k1, 31);
				k1 *= c2;
				h1 ^= k1;
			}

			h1 ^= length;
			h2 ^= length;
			h1 += h2;
			h2 += h1;

			h1 = fmix64(h1);
			h2 = fmix64(h2);
			return h1 + h2;
		}
	};

	template<uint64_types key_type_newer, uint64_t seed_new> struct rt_key_hasher<key_type_newer, seed_new> {
		template<uint64_types key_type> VC_INLINE uint64_t operator()(const key_type value) const noexcept {
			return mix64(static_cast<uint64_t>(value) + seed_new, mix64_multiplier);
		}
	};

	template<uint_types key_type_newer, uint64_t seed_new> struct rt_key_hasher<key_type_newer, seed_new> : public rt_key_hasher<uint64_t, seed_new> {
		template<uint_types key_type> VC_INLINE uint64_t operator()(const key_type value) const noexcept {
			return rt_key_hasher<uint64_t, seed_new>::operator()(static_cast<uint64_t>(value));
		}
	};

	struct pair_hasher_placeholder {};

	template<typename first_type, typename second_type> using pair_base_second_t =
		std::conditional_t<std::is_same_v<base_t<first_type>, base_t<second_type>>, pair_hasher_placeholder, second_type>;

	template<pair_types key_type_newer, uint64_t seed_new> struct rt_key_hasher<key_type_newer, seed_new>
		: public rt_key_hasher<typename key_type_newer::first_type, seed_new>,
		  public rt_key_hasher<pair_base_second_t<typename key_type_newer::first_type, typename key_type_newer::second_type>, seed_new> {
		template<pair_types key_type> VC_INLINE uint64_t operator()(const key_type& value) const noexcept {
			const uint64_t h1{ rt_key_hasher<typename key_type_newer::first_type, seed_new>::operator()(value.first) };
			const uint64_t h2{ rt_key_hasher<typename key_type_newer::second_type, seed_new>::operator()(value.second) };
			return h1 ^ h2;
		}
	};

	template<typename key_type_new, typename hasher_type_new> struct hasher : public hasher_type_new {
		using hasher_type = hasher_type_new;
		using key_type	  = key_type_new;

		template<typename key_type_newer> VC_INLINE uint64_t operator()(const key_type_newer& value) const noexcept {
			const hasher_type& underlying_hasher{ *this };
			if constexpr (requires { underlying_hasher(value); }) {
				return underlying_hasher(value);
			} else {
				return underlying_hasher(key_type(value));
			}
		}

		VC_INLINE static constexpr int8_t h2(const uint64_t hash_value) noexcept {
			return static_cast<int8_t>(hash_value & 0x7FULL);
		}

		VC_INLINE static constexpr uint64_t h1(const uint64_t hash_value) noexcept {
			return hash_value >> 7U;
		}
	};

}
