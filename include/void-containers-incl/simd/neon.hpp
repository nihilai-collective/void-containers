/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Nihilai Collective Corp
 * https://github.com/nihilai-collective/void-containers
 * include/void-containers-incl/simd/neon.hpp
 */
#pragma once

#include <void-containers-incl/config.hpp>
#include <void-containers-incl/simd/simd_types.hpp>

#if VC_NEON || VC_SVE2

namespace void_containers::internal {

	template<simd_128_types simd_class> VC_INLINE simd_int_128 load_values(const void* ptr) noexcept {
		return vld1q_u8(static_cast<const uint8_t*>(ptr));
	}

	template<simd_128_types simd_class> VC_INLINE simd_int_128 load_values_u(const void* ptr) noexcept {
		return vld1q_u8(static_cast<const uint8_t*>(ptr));
	}

	template<simd_128_types simd_class> VC_INLINE simd_int_128 op_set1(const uint8_t value) noexcept {
		return vdupq_n_u8(value);
	}

	template<simd_128_types simd_class> VC_INLINE simd_int_128 op_xor(const simd_int_128& lhs, const simd_int_128& rhs) noexcept {
		return veorq_u8(lhs, rhs);
	}

	template<simd_128_types simd_class> VC_INLINE simd_int_128 op_or(const simd_int_128& lhs, const simd_int_128& rhs) noexcept {
		return vorrq_u8(lhs, rhs);
	}

	template<simd_128_types simd_class> VC_INLINE bool op_test(const simd_int_128& value) noexcept {
		return vmaxvq_u8(value) != 0;
	}

	template<simd_128_types simd_class> VC_INLINE uint16_t op_cmp_eq(const simd_int_128& lhs, const simd_int_128& rhs) noexcept {
		alignas(16) static constexpr uint8_t lane_bits[16]{ 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80 };
		const uint8x16_t masked{ vandq_u8(vceqq_u8(lhs, rhs), vld1q_u8(lane_bits)) };
		uint8x16_t folded{ vpaddq_u8(masked, masked) };
		folded = vpaddq_u8(folded, folded);
		folded = vpaddq_u8(folded, folded);
		return vgetq_lane_u16(vreinterpretq_u16_u8(folded), 0);
	}

}

#endif
