/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Nihilai Collective Corp
 * https://github.com/nihilai-collective/void-containers
 * include/void-containers-incl/simd/fallback.hpp
 */
#pragma once

#include <void-containers-incl/config.hpp>
#include <void-containers-incl/simd/simd_types.hpp>

#if !VC_ANY_SIMD

namespace void_containers::internal {

	template<typename index_sequence> struct simd_x_ops;

	template<uint64_t... indices> struct simd_x_ops<std::integer_sequence<uint64_t, indices...>> {
		template<typename simd_type> VC_INLINE static constexpr simd_type set1(const uint8_t value) noexcept {
			simd_type result{};
			((result.values[indices] = value), ...);
			return result;
		}

		template<typename simd_type> VC_INLINE static constexpr simd_type bit_xor(const simd_type& lhs, const simd_type& rhs) noexcept {
			simd_type result{};
			((result.values[indices] = static_cast<uint8_t>(lhs.values[indices] ^ rhs.values[indices])), ...);
			return result;
		}

		template<typename simd_type> VC_INLINE static constexpr simd_type bit_or(const simd_type& lhs, const simd_type& rhs) noexcept {
			simd_type result{};
			((result.values[indices] = static_cast<uint8_t>(lhs.values[indices] | rhs.values[indices])), ...);
			return result;
		}

		template<typename simd_type> VC_INLINE static constexpr bool test(const simd_type& value) noexcept {
			return ((value.values[indices] != 0) || ...);
		}

		template<typename mask_type, typename simd_type> VC_INLINE static constexpr mask_type cmp_eq(const simd_type& lhs, const simd_type& rhs) noexcept {
			mask_type mask{};
			((mask = static_cast<mask_type>(mask | static_cast<mask_type>(static_cast<mask_type>(lhs.values[indices] == rhs.values[indices]) << indices))), ...);
			return mask;
		}
	};

	template<typename simd_class> using simd_x_ops_t = simd_x_ops<std::make_integer_sequence<uint64_t, simd_class::byte_size>>;

	template<typename simd_class> VC_INLINE typename simd_class::type load_values_u(const void* ptr) noexcept {
		typename simd_class::type result;
		std::memcpy(result.values, ptr, simd_class::byte_size);
		return result;
	}

	template<typename simd_class> VC_INLINE typename simd_class::type load_values(const void* ptr) noexcept {
		return load_values_u<simd_class>(ptr);
	}

	template<typename simd_class> VC_INLINE constexpr typename simd_class::type op_set1(const uint8_t value) noexcept {
		return simd_x_ops_t<simd_class>::template set1<typename simd_class::type>(value);
	}

	template<typename simd_class> VC_INLINE constexpr typename simd_class::type op_xor(const typename simd_class::type& lhs, const typename simd_class::type& rhs) noexcept {
		return simd_x_ops_t<simd_class>::bit_xor(lhs, rhs);
	}

	template<typename simd_class> VC_INLINE constexpr typename simd_class::type op_or(const typename simd_class::type& lhs, const typename simd_class::type& rhs) noexcept {
		return simd_x_ops_t<simd_class>::bit_or(lhs, rhs);
	}

	template<typename simd_class> VC_INLINE constexpr bool op_test(const typename simd_class::type& value) noexcept {
		return simd_x_ops_t<simd_class>::test(value);
	}

	template<typename simd_class>
	VC_INLINE constexpr typename simd_class::mask_type op_cmp_eq(const typename simd_class::type& lhs, const typename simd_class::type& rhs) noexcept {
		return simd_x_ops_t<simd_class>::template cmp_eq<typename simd_class::mask_type>(lhs, rhs);
	}

}

#endif
