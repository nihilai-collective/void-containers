# SPDX-License-Identifier: MIT
# Copyright (c) 2026 Nihilai Collective Corp
# https://github.com/nihilai-collective/void-containers
# CMakeLists.txt

cmake_minimum_required(VERSION 3.28)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(cmake/get_version.cmake)

project(void-containers
    VERSION ${CLEAN_VERSION}
    DESCRIPTION "A few containers, for storing things."
    LANGUAGES CXX
)

include(cmake/library_setup.cmake)

include(cmake/installation_setup.cmake)

if(VC_UNIT_TESTS)
    add_subdirectory(unit-tests)
endif()
