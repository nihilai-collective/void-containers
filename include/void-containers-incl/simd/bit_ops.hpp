/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Nihilai Collective Corp
 * https://github.com/nihilai-collective/void-containers
 * include/void-containers-incl/simd/bit_ops.hpp
 */
#pragma once

#include <void-containers-incl/config.hpp>

namespace void_containers::internal {

	template<std::unsigned_integral value_type> VC_INLINE constexpr value_type lzcnt(const value_type value) noexcept {
		return static_cast<value_type>(std::countl_zero(value));
	}

	template<std::unsigned_integral value_type> VC_INLINE constexpr value_type tzcnt(const value_type value) noexcept {
		return static_cast<value_type>(std::countr_zero(value));
	}

	template<std::unsigned_integral value_type> VC_INLINE constexpr value_type blsr(const value_type value) noexcept {
		return static_cast<value_type>(value & static_cast<value_type>(value - 1U));
	}

	VC_INLINE void prefetch(const void* ptr) noexcept {
#if VC_COMPILER_MSVC && VC_ARCH_X64
		_mm_prefetch(static_cast<const char*>(ptr), _MM_HINT_T0);
#elif VC_COMPILER_MSVC && VC_ARCH_ARM64
		__prefetch(ptr);
#elif VC_COMPILER_CLANG || VC_COMPILER_GCC
		__builtin_prefetch(ptr);
#else
		static_cast<void>(ptr);
#endif
	}

}
