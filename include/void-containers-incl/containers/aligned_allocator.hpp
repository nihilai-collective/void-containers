/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Nihilai Collective Corp
 * https://github.com/nihilai-collective/void-containers
 * include/void-containers-incl/containers/aligned_allocator.hpp
 */
#pragma once

#include <void-containers-incl/config.hpp>
#include <void-containers-incl/simd/vc_cpu_properties.hpp>

namespace void_containers {

	template<uint64_t multiple> VC_INLINE constexpr uint64_t round_up_to_multiple(const uint64_t value) noexcept {
		return (value + multiple - 1ULL) & ~(multiple - 1ULL);
	}

	inline constexpr uint64_t min_group_alignment{ 16 };
	inline constexpr uint64_t container_alignment{ internal::cpu_properties::get_value(internal::cpu_property_types::alignment) > min_group_alignment
			? internal::cpu_properties::get_value(internal::cpu_property_types::alignment)
			: min_group_alignment };

	template<typename value_type_new> class aligned_allocator {
	  public:
		using value_type = value_type_new;
		using pointer	 = value_type*;
		using size_type	 = uint64_t;

		static pointer allocate(const size_type count) {
			if (count == 0) {
				return nullptr;
			}
			const size_type bytes{ round_up_to_multiple<container_alignment>(count * sizeof(value_type)) };
			void* memory{};
			if (uses_pages(bytes)) {
				memory = allocate_pages(round_up_to_multiple<huge_page_size>(bytes));
			} else {
#if VC_PLATFORM_WINDOWS
				memory = _aligned_malloc(bytes, container_alignment);
#else
				memory = std::aligned_alloc(container_alignment, bytes);
#endif
			}
			if (memory == nullptr) {
				throw std::bad_alloc{};
			}
			return static_cast<pointer>(memory);
		}

		static void deallocate(pointer memory, const size_type count) noexcept {
			if (memory == nullptr) {
				return;
			}
			const size_type bytes{ round_up_to_multiple<container_alignment>(count * sizeof(value_type)) };
			if (uses_pages(bytes)) {
				deallocate_pages(memory, round_up_to_multiple<huge_page_size>(bytes));
			} else {
#if VC_PLATFORM_WINDOWS
				_aligned_free(memory);
#else
				std::free(memory);
#endif
			}
		}

	  private:
		static constexpr uint64_t huge_page_size{ 2ULL * 1024ULL * 1024ULL };
		static constexpr uint64_t huge_page_threshold{ huge_page_size * 2ULL };

		static constexpr bool uses_pages([[maybe_unused]] const size_type bytes) noexcept {
#if VC_PLATFORM_WINDOWS || VC_PLATFORM_LINUX || VC_PLATFORM_MAC
			return bytes >= huge_page_threshold;
#else
			return false;
#endif
		}

		static void* allocate_pages([[maybe_unused]] const size_type bytes) noexcept {
#if VC_PLATFORM_WINDOWS
			void* memory{ VirtualAlloc(nullptr, bytes, MEM_RESERVE | MEM_COMMIT | MEM_LARGE_PAGES, PAGE_READWRITE) };
			return memory != nullptr ? memory : VirtualAlloc(nullptr, bytes, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
#elif VC_PLATFORM_LINUX
			void* memory{ mmap(nullptr, bytes, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB, -1, 0) };
			if (memory != MAP_FAILED) {
				return memory;
			}
			memory = mmap(nullptr, bytes, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
			if (memory == MAP_FAILED) {
				return nullptr;
			}
			madvise(memory, bytes, MADV_HUGEPAGE);
			return memory;
#elif VC_PLATFORM_MAC
			void* memory{ mmap(nullptr, bytes, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0) };
			return memory == MAP_FAILED ? nullptr : memory;
#else
			return nullptr;
#endif
		}

		static void deallocate_pages([[maybe_unused]] void* memory, [[maybe_unused]] const size_type bytes) noexcept {
#if VC_PLATFORM_WINDOWS
			VirtualFree(memory, 0, MEM_RELEASE);
#elif VC_PLATFORM_LINUX || VC_PLATFORM_MAC
			munmap(memory, bytes);
#endif
		}
	};

}
