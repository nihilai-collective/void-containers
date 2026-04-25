/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Nihilai Collective Corp
 * https://github.com/nihilai-collective/void-containers
 * include/void-containers-incl/config.hpp
 */
#pragma once

#include <void-containers-incl/simd/vc_cpu_instructions.hpp>
#include <initializer_list>
#include <string_view>
#include <type_traits>
#include <stdexcept>
#include <algorithm>
#include <concepts>
#include <iterator>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <utility>
#include <limits>
#include <new>
#include <bit>

#if VC_ANY_AVX
	#include <immintrin.h>
#elif VC_NEON || VC_SVE2
	#include <arm_neon.h>
#endif

#if VC_COMPILER_MSVC
	#if VC_ARCH_X64
		#include <xmmintrin.h>
	#endif
	#include <intrin.h>
#endif

#if VC_PLATFORM_WINDOWS
	#include <Windows.h>
	#include <malloc.h>
#elif VC_PLATFORM_LINUX || VC_PLATFORM_MAC
	#include <sys/mman.h>
#endif

namespace void_containers {

	template<typename... arg_types> struct banned_reinterpret_cast {
		static_assert(sizeof...(arg_types) == 0,
			"Sorry, but reinterpret_cast is banned in this library! Only use our public-facing include <void-containers> in your code! Or, if you're inside our own headers, remove "
			"the std library include you just added and add it to config.hpp.");
	};

	template<typename... arg_types> struct banned_const_cast {
		static_assert(sizeof...(arg_types) == 0,
			"Sorry, but const_cast is banned in this library! Only use our public-facing include <void-containers> in your code! Or, if you're inside our own headers, remove "
			"the std library include you just added and add it to config.hpp.");
	};

	template<typename... arg_types> struct banned_dynamic_cast {
		static_assert(sizeof...(arg_types) == 0,
			"Sorry, but dynamic_cast is banned in this library! Only use our public-facing include <void-containers> in your code! Or, if you're inside our own headers, remove "
			"the std library include you just added and add it to config.hpp.");
	};

}

#define reinterpret_cast void_containers::banned_reinterpret_cast
#define const_cast void_containers::banned_const_cast
#define dynamic_cast void_containers::banned_dynamic_cast
