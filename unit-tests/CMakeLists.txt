# SPDX-License-Identifier: MIT
# Copyright (c) 2026 Nihilai Collective Corp
# https://github.com/nihilai-collective/void-containers
# unit-tests/CMakeLists.txt

add_executable(void-containers-unit-tests main.cpp sonnet_map_tests.cpp sonnet_set_tests.cpp)

set_source_files_properties(sonnet_map_tests.cpp PROPERTIES COMPILE_OPTIONS
    "$<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>:-Wno-everything>;$<$<CXX_COMPILER_ID:GNU>:-w>;$<$<CXX_COMPILER_ID:MSVC>:/w>"
)

include(FetchContent)

FetchContent_Declare(
  rt-ut
  GIT_REPOSITORY https://github.com/nihilai-collective/rt-ut.git
  GIT_TAG main
  GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(rt-ut)

target_link_libraries(void-containers-unit-tests
    PUBLIC
        void-containers::void-containers rt-ut::rt-ut
)

option(VC_ASAN  "Enable AddressSanitizer"            OFF)
option(VC_UBSAN "Enable UndefinedBehaviorSanitizer"  OFF)

set(VC_ASAN_EFFECTIVE  ${VC_ASAN})
set(VC_UBSAN_EFFECTIVE ${VC_UBSAN})

if(APPLE AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(VC_ASAN_EFFECTIVE OFF)
    set(VC_UBSAN_EFFECTIVE OFF)
    message(STATUS "Disabling sanitizers: not supported with GCC on macOS.")
endif()

if(VC_UBSAN_EFFECTIVE AND CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    message(WARNING
        "VC_UBSAN has no effect with MSVC (no equivalent runtime). "
        "Use Clang-cl or a Clang/GCC build to get UBSan coverage.")
    set(VC_UBSAN_EFFECTIVE OFF)
endif()

set(VC_MSVC_COMPILER_OPTIONS
    $<$<AND:$<CONFIG:Debug>,$<NOT:$<BOOL:${VC_ASAN_EFFECTIVE}>>>:/Od;/Zi;/RTC1>
    $<$<AND:$<CONFIG:Debug>,$<BOOL:${VC_ASAN_EFFECTIVE}>>:/Od;/Zi>
    $<$<BOOL:${VC_ASAN_EFFECTIVE}>:/fsanitize=address>
    /wd4324
    /wd4820
    /wd5039
    /wd4710
    /wd5045
    /wd4711
    /wd4623
    /wd4626
    /wd4686
    /wd4865
    /wd5027
    /Wall
    /W4
    /bigobj
    /WX
)

set(VC_CLANG_COMPILER_OPTIONS
    $<$<AND:$<CONFIG:Release>,$<BOOL:${VC_ASAN_EFFECTIVE}>>:-O1>
    $<$<CONFIG:Debug>:-O0;-g;-fno-omit-frame-pointer>
    $<$<BOOL:${VC_ASAN_EFFECTIVE}>:-fsanitize=address;-fno-omit-frame-pointer;-fno-optimize-sibling-calls;-fsanitize-address-use-after-scope;-U_FORTIFY_SOURCE;-D_FORTIFY_SOURCE=0>
    $<$<BOOL:${VC_UBSAN_EFFECTIVE}>:-fsanitize=undefined;-fno-sanitize-recover=all>
    $<$<AND:$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>,$<NOT:$<VERSION_LESS:$<CXX_COMPILER_VERSION>,23>>>:-Wno-lifetime-safety-lifetimebound-violation;-Wno-lifetime-safety-intra-tu-suggestions>
    $<$<AND:$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>,$<NOT:$<VERSION_LESS:$<CXX_COMPILER_VERSION>,23>>,$<VERSION_LESS:$<CXX_COMPILER_VERSION>,24>>:-Wno-lifetime-safety-invalidation>
    $<$<AND:$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>,$<NOT:$<VERSION_LESS:$<CXX_COMPILER_VERSION>,24>>>:-Wno-lifetime-safety-experimental-invalidation;-Wno-lifetime-safety-experimental-field-origin-escape>
    -Weverything
    -Wpedantic
    -Werror
    -Wno-unsafe-buffer-usage
    -Wno-c++20-extensions
    -Wno-c++20-compat
    -Wextra
    -Wall
    -Wno-c++98-compat-pedantic
    -Wno-c++98-compat
    -Wno-keyword-macro
    -Wno-padded
)

set(VC_GNU_COMPILER_OPTIONS
    $<$<CONFIG:Debug>:-O0;-g>
    $<$<BOOL:${VC_ASAN_EFFECTIVE}>:-fsanitize=address;-fsanitize-address-use-after-scope;-U_FORTIFY_SOURCE;-D_FORTIFY_SOURCE=0>
    $<$<BOOL:${VC_UBSAN_EFFECTIVE}>:-fsanitize=undefined;-fno-sanitize-recover=all>
    -Wpedantic
    -Wno-ignored-attributes
    -Werror
    -Wextra
    -Wall
)

target_compile_options(void-containers-unit-tests PUBLIC
    $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>:${VC_CLANG_COMPILER_OPTIONS}>
    $<$<CXX_COMPILER_ID:GNU>:${VC_GNU_COMPILER_OPTIONS}>
    $<$<CXX_COMPILER_ID:MSVC>:${VC_MSVC_COMPILER_OPTIONS}>
)

set(VC_MSVC_LINK_OPTIONS
    $<$<BOOL:${VC_ASAN_EFFECTIVE}>:/DEBUG>
)

set(VC_CLANG_LINK_OPTIONS
    $<$<BOOL:${VC_ASAN_EFFECTIVE}>:-fsanitize=address>
    $<$<BOOL:${VC_UBSAN_EFFECTIVE}>:-fsanitize=undefined>
)

set(VC_GNU_LINK_OPTIONS
    $<$<BOOL:${VC_ASAN_EFFECTIVE}>:-fsanitize=address>
    $<$<BOOL:${VC_UBSAN_EFFECTIVE}>:-fsanitize=undefined>
    $<$<AND:$<PLATFORM_ID:Linux>,$<BOOL:${VC_ASAN_EFFECTIVE}>>:-static-libasan>
)

target_link_options(void-containers-unit-tests PUBLIC
    $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>:${VC_CLANG_LINK_OPTIONS}>
    $<$<CXX_COMPILER_ID:GNU>:${VC_GNU_LINK_OPTIONS}>
    $<$<CXX_COMPILER_ID:MSVC>:${VC_MSVC_LINK_OPTIONS}>
)

install(
    FILES
        $<TARGET_FILE:void-containers-unit-tests>
    DESTINATION bin
    OPTIONAL
)
