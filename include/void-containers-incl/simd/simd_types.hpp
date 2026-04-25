/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Nihilai Collective Corp
 * https://github.com/nihilai-collective/void-containers
 * include/void-containers-incl/simd/simd_types.hpp
 */
#pragma once

#include <void-containers-incl/config.hpp>
#include <void-containers-incl/simd/vc_cpu_properties.hpp>

#if !defined(VC_SIMD_HAS_256)
	#define VC_SIMD_HAS_256 (VC_AVX2 || VC_AVX512 || !VC_ANY_SIMD)
#endif

#if !defined(VC_SIMD_HAS_512)
	#define VC_SIMD_HAS_512 (VC_AVX512 || !VC_ANY_SIMD)
#endif

namespace void_containers::internal {

	template<uint64_t byte_size_new> struct simd_x {
		static constexpr uint64_t byte_size{ byte_size_new };
		alignas(byte_size_new) uint8_t values[byte_size_new];
	};

#if VC_ANY_AVX
	using simd_int_128 = __m128i;
#elif VC_NEON || VC_SVE2
	using simd_int_128 = uint8x16_t;
#else
	using simd_int_128 = simd_x<16>;
#endif

	struct simd_int_128_t {
		using type		= simd_int_128;
		using mask_type = uint16_t;
		static constexpr uint64_t byte_size{ 16 };
	};

	template<typename value_type>
	concept simd_128_types = std::same_as<value_type, simd_int_128_t>;

#if VC_SIMD_HAS_256
	#if VC_ANY_SIMD
	using simd_int_256 = __m256i;
	#else
	using simd_int_256 = simd_x<32>;
	#endif

	struct simd_int_256_t {
		using type		= simd_int_256;
		using mask_type = uint32_t;
		static constexpr uint64_t byte_size{ 32 };
	};

	template<typename value_type>
	concept simd_256_types = std::same_as<value_type, simd_int_256_t>;
#endif

#if VC_SIMD_HAS_512
	#if VC_ANY_SIMD
	using simd_int_512 = __m512i;
	#else
	using simd_int_512 = simd_x<64>;
	#endif

	struct simd_int_512_t {
		using type		= simd_int_512;
		using mask_type = uint64_t;
		static constexpr uint64_t byte_size{ 64 };
	};

	template<typename value_type>
	concept simd_512_types = std::same_as<value_type, simd_int_512_t>;
#endif

}
