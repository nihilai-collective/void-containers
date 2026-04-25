![Lines of Code](https://img.shields.io/endpoint?url=https://raw.githubusercontent.com/nihilai-collective/void-containers/badges/loc.json&style=plastic)
[![License](https://img.shields.io/badge/license-MIT-blue.svg?style=plastic)](License.md)
[![C++](https://img.shields.io/badge/C%2B%2B-23-blue.svg?style=plastic)](https://isocpp.org/)
[![Header-only](https://img.shields.io/badge/header--only-yes-brightgreen.svg?style=plastic)](include/)

---
## Compiler Support
![MSVC](https://img.shields.io/github/actions/workflow/status/nihilai-collective/void-containers/unit-tests.yml?style=plastic&logo=microsoft&logoColor=green&label=MSVC&labelColor=pewter&color=blue&branch=main)
![GCC](https://img.shields.io/github/actions/workflow/status/nihilai-collective/void-containers/unit-tests.yml?style=plastic&logo=linux&logoColor=green&label=GCC&labelColor=pewter&color=blue&branch=main)
![CLANG](https://img.shields.io/github/actions/workflow/status/nihilai-collective/void-containers/unit-tests.yml?style=plastic&logo=apple&logoColor=green&label=CLANG&labelColor=pewter&color=blue&branch=main)

## Operating System Support
![Windows](https://img.shields.io/github/actions/workflow/status/nihilai-collective/void-containers/unit-tests.yml?style=plastic&logo=microsoft&logoColor=green&label=Windows&labelColor=pewter&color=blue&branch=main)
![Linux](https://img.shields.io/github/actions/workflow/status/nihilai-collective/void-containers/unit-tests.yml?style=plastic&logo=linux&logoColor=green&label=Linux&labelColor=pewter&color=blue&branch=main)
![Mac](https://img.shields.io/github/actions/workflow/status/nihilai-collective/void-containers/unit-tests.yml?style=plastic&logo=apple&logoColor=green&label=MacOS&labelColor=pewter&color=blue&branch=main)

---

A few containers, for storing things.
===============================================

void-containers is a header-only C++23 library of open-addressing hash containers built on SIMD group probing:

* **[`sonnet_map`](./include/void-containers-incl/containers/sonnet_map.hpp):** a key-to-value hash map.
* **[`sonnet_set`](./include/void-containers-incl/containers/sonnet_set.hpp):** a hash set with read-only iteration.

Both are thin layers over the same [table implementation](./include/void-containers-incl/containers/table_operations.hpp), so they share their probing, growth, and erase behavior.

---

## Quick Example

```cpp
#include <void-containers>
#include <cstdint>
#include <iostream>
#include <string>

struct is_cold {
    static bool impl(const void_containers::pair<std::string, uint32_t>& entry) {
        return entry.second < 10U;
    }
};

int32_t main() {
    void_containers::sonnet_map<std::string, uint32_t> scores{};
    scores["gecko"] = 42U;
    scores.insert("parrot", 7U);
    scores.insert_or_assign("goldfish", 12U);

    if (auto found = scores.find("gecko"); found != scores.end()) {
        std::cout << found->first << ": " << found->second << '\n';
    }

    void_containers::erase_if(scores, &is_cold::impl);

    void_containers::sonnet_set<uint32_t> seen{};
    const bool first_visit{ seen.insert(1234U).second };

    return first_visit && !scores.contains("parrot") ? 0 : 1;
}
```

Warning: Include only `<void-containers>`. Its internal headers define `reinterpret_cast`, `const_cast`, and `dynamic_cast` as macros that fail compilation, and only the public header removes them again.

---

## Features

### SIMD Group Probing
Each slot has one control byte recording whether it is empty, deleted, or full; a full slot's byte also holds 7 bits of the key's hash. Lookups compare a group of 16 control bytes at once, so the key itself is only compared for slots whose hash bits already match. A key's remaining hash bits select its starting group, and probing advances one group at a time until it reaches a group with an empty slot.

### One Group Width, Three Backends
Groups are always 16 bytes wide, whichever instruction set is detected. `simd_int_128_t` is backed by:

| Build target | Implementation |
| :--- | :--- |
| AVX, AVX2, or AVX-512 | [SSE-width intrinsics](./include/void-containers-incl/simd/avx.hpp) |
| NEON or SVE2 | [NEON intrinsics](./include/void-containers-incl/simd/neon.hpp) |
| Anything else | [Portable scalar fallback](./include/void-containers-incl/simd/fallback.hpp) |

### Built-In Hashing
`rt_key_hasher` covers strings, unsigned integers, and pairs of supported types. String hashing is MurmurHash3-style, and integer hashing uses a 128-bit multiply mix. The default seed is a fixed constant, so these hashers are not intended to resist deliberately crafted collisions. To change the seed, pass `rt_key_hasher<key_type, your_seed>` as the hasher.

### Heterogeneous Lookup
A `std::string`-keyed container can look up, and test for, a `std::string_view` or string literal without constructing a temporary `std::string`. Other key types are accepted where they can be converted to the container's key type.

### Custom Hashers and Key Equality
Both containers take their hasher and key equality as template parameters:

```cpp
void_containers::sonnet_map<std::string, uint32_t, case_blind_hasher, case_blind_equal> map{};
```

Tables use the low 7 bits of the hash as the control byte and the bits above them to select a group, so a custom hasher needs to distribute well across both.

### Memory Layout and Allocation
Control bytes and elements live in two separate allocations, aligned to at least 16 bytes and to the detected CPU alignment when that is larger. Allocations of 4 MiB or more go directly to the operating system: Windows and Linux try huge pages first and fall back to regular pages, while macOS uses regular `mmap`. An empty container allocates nothing until its first insertion.

### Growth and Erasure
Capacity is always a power of two. A table grows when its live elements plus tombstones reach 7/8 of capacity. Erasing leaves a tombstone that a later insertion can reuse, and a rehash clears them all.

### Introspection and Hot-Path Helpers
- `reserve` and `rehash` control capacity up front, as does the size hint on the constructor.
- `prefetch` loads a key's first control group into cache ahead of a lookup.
- `total_memory_usage` reports the footprint of the table, including the heap contents of keys and values that expose `size()`.

---

## API Overview

| Operation | `sonnet_map` | `sonnet_set` |
| :--- | :---: | :---: |
| `operator[]`, `at` | yes | |
| `try_emplace`, `insert_or_assign` | yes | |
| `insert`, `emplace`, `erase` | yes | yes |
| `find`, `contains`, `count`, `prefetch` | yes | yes |
| `begin`, `end`, `cbegin`, `cend` | yes | yes |
| `size`, `empty`, `capacity`, `clear`, `swap` | yes | yes |
| `reserve`, `rehash`, `total_memory_usage` | yes | yes |
| `operator==` | yes | yes |

The free function `void_containers::erase_if(container, predicate)` works with both and returns the number of elements removed.

---

## Differences From the Standard Containers

These containers look familiar but are not drop-in replacements for `std::unordered_map` and `std::unordered_set`:

- **Signed integer keys need a custom hasher.** `rt_key_hasher` only provides unsigned integers, strings, and pairs.
- **Map elements are `void_containers::pair<key, value>`.** The `first` member is not `const`, but changing a key through an iterator corrupts the table.
- **`erase` returns `bool`** for a key or a single iterator. The range overload returns its `last` iterator.
- **Most operations are `noexcept`,** including inserts and copies, so a failed allocation during one terminates the program. `at` throws `std::out_of_range` for a missing key.
- **Rehashing invalidates all iterators and references.** Any insertion can trigger one unless the capacity was reserved in advance.
- **Iteration order is unspecified** and can change after a rehash.
- **There is no allocator parameter.** All storage goes through the library's `aligned_allocator`.

---

## Installation

### CMake FetchContent

```cmake
include(FetchContent)

FetchContent_Declare(
    void-containers
    GIT_REPOSITORY https://github.com/nihilai-collective/void-containers.git
    GIT_TAG main
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(void-containers)

target_link_libraries(your_target PRIVATE void-containers::void-containers)
```

### Installed Package

```bash
cmake -S . -B Build -DCMAKE_BUILD_TYPE=Release
cmake --install Build
```

```cmake
find_package(void-containers CONFIG REQUIRED)
target_link_libraries(your_target PRIVATE void-containers::void-containers)
```

Link the CMake target rather than copying the headers into a project. The target supplies required definitions such as `VC_INLINE` and the platform macros, and configuring generates the CPU headers described below.

---

## CPU Architecture Support

CPU detection is handled by [voided-hw-detection](https://github.com/nihilai-collective/voided-hw-detection), which is downloaded at configure time. Configuring generates `include/void-containers-incl/simd/vc_cpu_properties.hpp` and `vc_cpu_instructions.hpp` from the result.

The instruction set is fixed at configure time and nothing is selected at run time, so configure for the lowest tier you need when distributing to mixed hardware.

| CMake option | Effect |
| :--- | :--- |
| `-DVC_SIMD_TIER=AVX2` | Builds for a tier below the detected one |
| `-DVC_CPU_INSTRUCTIONS=0` | Forces the pure-scalar fallback |
| `-DVC_DETECT_CPU=FALSE` | Skips hardware detection and uses its fallback values |

---

## Testing

The [unit tests](./unit-tests) cover the SIMD primitives, bit operations, key comparison, and both containers, across more than 200 test cases, using [rt-ut](https://github.com/nihilai-collective/rt-ut).

Every push runs them on Ubuntu (Clang/GCC), macOS (Clang/GCC), and Windows (MSVC), once with the detected instruction set and once as a pure-scalar build. AddressSanitizer and UndefinedBehaviorSanitizer are enabled where the toolchain supports them: MSVC has no UBSan, and GCC on macOS runs without sanitizers.

To build the tests on Windows:

```bash
cmake --preset void-containers-Windows-Release-Tests
cmake --build --preset void-containers-Windows-Release-Tests
```

On other platforms:

```bash
cmake -S . -B Build -DCMAKE_BUILD_TYPE=Release -DVC_UNIT_TESTS=TRUE
cmake --build Build
```

Add `-DVC_ASAN=TRUE` or `-DVC_UBSAN=TRUE` for sanitizers. The Windows presets also include Debug, ASan, and scalar-fallback variants; see [CMakePresets.json](./CMakePresets.json).

---

## Requirements

- CMake 3.28 or later
- A C++23 compiler (MSVC, GCC, or Clang; CI tracks recent releases of each)
- Network access at configure time to fetch voided-hw-detection, plus rt-ut when building the tests

---

## License

This library is licensed under the MIT License. See the [LICENSE](License.md) file for details.

---

## Contributing

Contributions are welcome! Please feel free to submit pull requests or open issues on GitHub.

---

**Star this repository if you find it useful!** ⭐
