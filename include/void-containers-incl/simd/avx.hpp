/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Nihilai Collective Corp
 * https://github.com/nihilai-collective/void-containers
 * include/void-containers-incl/simd/avx.hpp
 */
#pragma once

#include <void-containers-incl/config.hpp>
#include <void-containers-incl/simd/simd_types.hpp>

#if VC_ANY_AVX

namespace void_containers::internal {

	template<simd_128_types simd_class> VC_INLINE simd_int_128 load_values(const void* ptr) noexcept {
		return _mm_load_si128(static_cast<const __m128i*>(ptr));
	}

	template<simd_128_types simd_class> VC_INLINE simd_int_128 load_values_u(const void* ptr) noexcept {
		return _mm_loadu_si128(static_cast<const __m128i*>(ptr));
	}

	template<simd_128_types simd_class> VC_INLINE simd_int_128 op_set1(const uint8_t value) noexcept {
		return _mm_set1_epi8(static_cast<char>(value));
	}

	template<simd_128_types simd_class> VC_INLINE simd_int_128 op_xor(const simd_int_128& lhs, const simd_int_128& rhs) noexcept {
		return _mm_xor_si128(lhs, rhs);
	}

	template<simd_128_types simd_class> VC_INLINE simd_int_128 op_or(const simd_int_128& lhs, const simd_int_128& rhs) noexcept {
		return _mm_or_si128(lhs, rhs);
	}

	template<simd_128_types simd_class> VC_INLINE bool op_test(const simd_int_128& value) noexcept {
		return !_mm_testz_si128(value, value);
	}

	template<simd_128_types simd_class> VC_INLINE uint16_t op_cmp_eq(const simd_int_128& lhs, const simd_int_128& rhs) noexcept {
		return static_cast<uint16_t>(_mm_movemask_epi8(_mm_cmpeq_epi8(lhs, rhs)));
	}

	#if VC_SIMD_HAS_256

	template<simd_256_types simd_class> VC_INLINE simd_int_256 load_values(const void* ptr) noexcept {
		return _mm256_load_si256(static_cast<const __m256i*>(ptr));
	}

	template<simd_256_types simd_class> VC_INLINE simd_int_256 load_values_u(const void* ptr) noexcept {
		return _mm256_loadu_si256(static_cast<const __m256i*>(ptr));
	}

	template<simd_256_types simd_class> VC_INLINE simd_int_256 op_set1(const uint8_t value) noexcept {
		return _mm256_set1_epi8(static_cast<char>(value));
	}

	template<simd_256_types simd_class> VC_INLINE simd_int_256 op_xor(const simd_int_256& lhs, const simd_int_256& rhs) noexcept {
		return _mm256_xor_si256(lhs, rhs);
	}

	template<simd_256_types simd_class> VC_INLINE simd_int_256 op_or(const simd_int_256& lhs, const simd_int_256& rhs) noexcept {
		return _mm256_or_si256(lhs, rhs);
	}

	template<simd_256_types simd_class> VC_INLINE bool op_test(const simd_int_256& value) noexcept {
		return !_mm256_testz_si256(value, value);
	}

	template<simd_256_types simd_class> VC_INLINE uint32_t op_cmp_eq(const simd_int_256& lhs, const simd_int_256& rhs) noexcept {
		return static_cast<uint32_t>(_mm256_movemask_epi8(_mm256_cmpeq_epi8(lhs, rhs)));
	}

	#endif

	#if VC_SIMD_HAS_512

	template<simd_512_types simd_class> VC_INLINE simd_int_512 load_values(const void* ptr) noexcept {
		return _mm512_load_si512(ptr);
	}

	template<simd_512_types simd_class> VC_INLINE simd_int_512 load_values_u(const void* ptr) noexcept {
		return _mm512_loadu_si512(ptr);
	}

	template<simd_512_types simd_class> VC_INLINE simd_int_512 op_set1(const uint8_t value) noexcept {
		return _mm512_set1_epi8(static_cast<char>(value));
	}

	template<simd_512_types simd_class> VC_INLINE simd_int_512 op_xor(const simd_int_512& lhs, const simd_int_512& rhs) noexcept {
		return _mm512_xor_si512(lhs, rhs);
	}

	template<simd_512_types simd_class> VC_INLINE simd_int_512 op_or(const simd_int_512& lhs, const simd_int_512& rhs) noexcept {
		return _mm512_or_si512(lhs, rhs);
	}

	template<simd_512_types simd_class> VC_INLINE bool op_test(const simd_int_512& value) noexcept {
		return _mm512_test_epi64_mask(value, value) != 0;
	}

	template<simd_512_types simd_class> VC_INLINE uint64_t op_cmp_eq(const simd_int_512& lhs, const simd_int_512& rhs) noexcept {
		return static_cast<uint64_t>(_mm512_cmpeq_epi8_mask(lhs, rhs));
	}

	#endif

}

#endif
