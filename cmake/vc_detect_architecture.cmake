# SPDX-License-Identifier: MIT
# Copyright (c) 2026 Nihilai Collective Corp
# https://github.com/nihilai-collective/void-containers
# cmake/vc_detect_architecture.cmake

include(FetchContent)

FetchContent_Declare(
    voided_hw_detection
    GIT_REPOSITORY https://github.com/nihilai-collective/voided-hw-detection.git
    GIT_TAG main
)
FetchContent_MakeAvailable(voided_hw_detection)

voided_hw_detect_cpu(
    PREFIX VC
    NO_DEFINITIONS
    TEMPLATE "${CMAKE_CURRENT_SOURCE_DIR}/cmake/vc_cpu_properties.hpp.in"
    HEADER "${CMAKE_CURRENT_SOURCE_DIR}/include/void-containers-incl/simd/vc_cpu_properties.hpp"
)

math(EXPR VC_LZCNT "(${VC_SELECTED_CPU_INSTRUCTIONS} >> 0) & 1" OUTPUT_FORMAT DECIMAL)
math(EXPR VC_POPCNT "(${VC_SELECTED_CPU_INSTRUCTIONS} >> 1) & 1" OUTPUT_FORMAT DECIMAL)
math(EXPR VC_BMI "(${VC_SELECTED_CPU_INSTRUCTIONS} >> 2) & 1" OUTPUT_FORMAT DECIMAL)
math(EXPR VC_CLMUL "(${VC_SELECTED_CPU_INSTRUCTIONS} >> 3) & 1" OUTPUT_FORMAT DECIMAL)
math(EXPR VC_NEON "(${VC_SELECTED_CPU_INSTRUCTIONS} >> 4) & 1" OUTPUT_FORMAT DECIMAL)
math(EXPR VC_AVX "(${VC_SELECTED_CPU_INSTRUCTIONS} >> 5) & 1" OUTPUT_FORMAT DECIMAL)
math(EXPR VC_AVX2 "(${VC_SELECTED_CPU_INSTRUCTIONS} >> 6) & 1" OUTPUT_FORMAT DECIMAL)
math(EXPR VC_AVX512 "(${VC_SELECTED_CPU_INSTRUCTIONS} >> 7) & 1" OUTPUT_FORMAT DECIMAL)
math(EXPR VC_SVE2 "(${VC_SELECTED_CPU_INSTRUCTIONS} >> 8) & 1" OUTPUT_FORMAT DECIMAL)

file(WRITE "${CMAKE_CURRENT_SOURCE_DIR}/include/void-containers-incl/simd/vc_cpu_instructions.hpp" "/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Nihilai Collective Corp
 * https://github.com/nihilai-collective/void-containers
 * include/void-containers-incl/simd/vc_cpu_instructions.hpp
 */
#pragma once

#undef VC_SVE2_VECTOR_BITS
#define VC_SVE2_VECTOR_BITS ${VC_SELECTED_SVE2_VECTOR_BITS}

#if !defined(VC_LZCNT)
	#define VC_LZCNT ${VC_LZCNT}
#endif

#if !defined(VC_POPCNT)
	#define VC_POPCNT ${VC_POPCNT}
#endif

#if !defined(VC_BMI)
	#define VC_BMI ${VC_BMI}
#endif

#if !defined(VC_CLMUL)
	#define VC_CLMUL ${VC_CLMUL}
#endif

#if !defined(VC_NEON)
	#define VC_NEON ${VC_NEON}
#endif

#if !defined(VC_AVX)
	#define VC_AVX ${VC_AVX}
#endif

#if !defined(VC_AVX2)
	#define VC_AVX2 ${VC_AVX2}
#endif

#if !defined(VC_AVX512)
	#define VC_AVX512 ${VC_AVX512}
#endif

#if !defined(VC_SVE2)
	#define VC_SVE2 ${VC_SVE2}
#endif

#if !defined(VC_ANY_AVX)
	#define VC_ANY_AVX (VC_AVX || VC_AVX2 || VC_AVX512)
#endif

#if !defined(VC_ANY_SIMD)
	#define VC_ANY_SIMD (VC_AVX || VC_AVX2 || VC_AVX512 || VC_NEON || VC_SVE2)
#endif

#if VC_NEON && VC_SVE2
	#error \"VC_NEON and VC_SVE2 are mutually exclusive backends - every SVE2 part also reports NEON, so exactly one must be selected.\"
#endif

#if VC_SVE2 && VC_SVE2_VECTOR_BITS == 0
	#error \"VC_SVE2 is selected but VC_SVE2_VECTOR_BITS is 0 - the fixed-length SVE2 typedefs require a measured vector length.\"
#endif
")
