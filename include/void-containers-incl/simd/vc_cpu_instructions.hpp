/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Nihilai Collective Corp
 * https://github.com/nihilai-collective/void-containers
 * include/void-containers-incl/simd/vc_cpu_instructions.hpp
 */
#pragma once

#undef VC_SVE2_VECTOR_BITS
#define VC_SVE2_VECTOR_BITS 0

#if !defined(VC_LZCNT)
	#define VC_LZCNT 1
#endif

#if !defined(VC_POPCNT)
	#define VC_POPCNT 1
#endif

#if !defined(VC_BMI)
	#define VC_BMI 1
#endif

#if !defined(VC_CLMUL)
	#define VC_CLMUL 1
#endif

#if !defined(VC_NEON)
	#define VC_NEON 0
#endif

#if !defined(VC_AVX)
	#define VC_AVX 1
#endif

#if !defined(VC_AVX2)
	#define VC_AVX2 1
#endif

#if !defined(VC_AVX512)
	#define VC_AVX512 0
#endif

#if !defined(VC_SVE2)
	#define VC_SVE2 0
#endif

#if !defined(VC_ANY_AVX)
	#define VC_ANY_AVX (VC_AVX || VC_AVX2 || VC_AVX512)
#endif

#if !defined(VC_ANY_SIMD)
	#define VC_ANY_SIMD (VC_AVX || VC_AVX2 || VC_AVX512 || VC_NEON || VC_SVE2)
#endif

#if VC_NEON && VC_SVE2
	#error "VC_NEON and VC_SVE2 are mutually exclusive backends - every SVE2 part also reports NEON, so exactly one must be selected."
#endif

#if VC_SVE2 && VC_SVE2_VECTOR_BITS == 0
	#error "VC_SVE2 is selected but VC_SVE2_VECTOR_BITS is 0 - the fixed-length SVE2 typedefs require a measured vector length."
#endif
