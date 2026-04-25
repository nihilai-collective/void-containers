/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Nihilai Collective Corp
 * https://github.com/nihilai-collective/void-containers
 * include/void-containers-incl/simd/compare.hpp
 */
#pragma once

#include <void-containers-incl/config.hpp>
#include <void-containers-incl/simd/avx.hpp>
#include <void-containers-incl/simd/neon.hpp>
#include <void-containers-incl/simd/fallback.hpp>

namespace void_containers::internal {

	struct comparison {
		VC_INLINE static bool equal_chunk(const void* lhs, const void* rhs) noexcept {
			return op_cmp_eq<simd_int_128_t>(load_values_u<simd_int_128_t>(lhs), load_values_u<simd_int_128_t>(rhs)) == uint16_t{ 0xFFFF };
		}

		VC_INLINE static bool compare(const void* lhs_new, const void* rhs_new, const uint64_t length) noexcept {
			if (length < simd_int_128_t::byte_size) {
				return std::memcmp(lhs_new, rhs_new, length) == 0;
			}
			const char* lhs{ static_cast<const char*>(lhs_new) };
			const char* rhs{ static_cast<const char*>(rhs_new) };
			const uint64_t last_chunk{ length - simd_int_128_t::byte_size };
			for (uint64_t index = 0; index < last_chunk; index += simd_int_128_t::byte_size) {
				if (!equal_chunk(lhs + index, rhs + index)) {
					return false;
				}
			}
			return equal_chunk(lhs + last_chunk, rhs + last_chunk);
		}
	};

}
