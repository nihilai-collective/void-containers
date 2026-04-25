/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Nihilai Collective Corp
 * https://github.com/nihilai-collective/void-containers
 * include/void-containers-incl/containers/table_storage.hpp
 */
#pragma once

#include <void-containers-incl/config.hpp>
#include <void-containers-incl/containers/aligned_allocator.hpp>
#include <void-containers-incl/containers/table_control.hpp>

namespace void_containers {

	template<typename value_type_new> struct table_storage {
		using value_type			   = value_type_new;
		using size_type				   = uint64_t;
		using ctrl_byte_allocator_type = aligned_allocator<ctrl_byte_type>;
		using element_allocator_type   = aligned_allocator<value_type>;

		table_storage(const size_type hint) noexcept {
			if (hint != 0) {
				allocate_table(get_new_size(hint));
			}
		}

		table_storage& operator=(table_storage&& other) noexcept {
			if (this != &other) {
				release();
				swap_storage(other);
			}
			return *this;
		}

		table_storage(table_storage&& other) noexcept {
			swap_storage(other);
		}

		table_storage& operator=(const table_storage& other) noexcept {
			if (this != &other) {
				release();
				if (other.total_size != 0) {
					allocate_table(other.total_size);
					current_size	= other.current_size;
					tombstone_count = other.tombstone_count;
					std::copy(other.ctrl_bytes_ptr, other.ctrl_bytes_ptr + other.total_size, ctrl_bytes_ptr);
					for (size_type index = 0; index < other.total_size; ++index) {
						if (ctrl_constants::is_full(other.ctrl_bytes_ptr[index])) {
							new (&element_ptr[index]) value_type(other.element_ptr[index]);
						}
					}
				}
			}
			return *this;
		}

		table_storage(const table_storage& other) noexcept {
			*this = other;
		}

		VC_INLINE size_type growth_limit() const noexcept {
			return total_size - total_size / 8ULL;
		}

		VC_INLINE bool should_resize() const noexcept {
			return current_size + tombstone_count >= growth_limit();
		}

		VC_INLINE bool empty() const noexcept {
			return current_size == 0;
		}

		VC_INLINE size_type size() const noexcept {
			return current_size;
		}

		VC_INLINE size_type capacity() const noexcept {
			return total_size;
		}

		ctrl_byte_type* ctrl_bytes_ptr{ table_empty_group };
		value_type* element_ptr{ nullptr };
		size_type tombstone_count{};
		size_type current_size{};
		size_type total_size{};
		size_type num_groups{ 1 };

	  protected:
		static size_type get_new_size(const size_type new_size) noexcept {
			return std::bit_ceil(round_up_to_multiple<container_alignment>((new_size + 1ULL) * 2ULL));
		}

		static constexpr size_type ctrl_byte_count(const size_type capacity_new) noexcept {
			return capacity_new + 1ULL;
		}

		void swap_storage(table_storage& other) noexcept {
			std::swap(ctrl_bytes_ptr, other.ctrl_bytes_ptr);
			std::swap(element_ptr, other.element_ptr);
			std::swap(tombstone_count, other.tombstone_count);
			std::swap(current_size, other.current_size);
			std::swap(total_size, other.total_size);
			std::swap(num_groups, other.num_groups);
		}

		void allocate_table(const size_type new_size) noexcept {
			ctrl_bytes_ptr = ctrl_byte_allocator_type::allocate(ctrl_byte_count(new_size));
			element_ptr	   = element_allocator_type::allocate(new_size);
			std::fill(ctrl_bytes_ptr, ctrl_bytes_ptr + new_size, ctrl_constants::empty_val);
			ctrl_bytes_ptr[new_size] = ctrl_constants::sentinel_val;
			total_size				 = new_size;
			num_groups				 = new_size / table_group_width;
			current_size			 = 0;
			tombstone_count			 = 0;
		}

		void destroy_elements() noexcept {
			for (size_type index = 0; index < total_size; ++index) {
				if (ctrl_constants::is_full(ctrl_bytes_ptr[index])) {
					element_ptr[index].~value_type();
				}
			}
		}

		void release() noexcept {
			if (total_size != 0) {
				destroy_elements();
				ctrl_byte_allocator_type::deallocate(ctrl_bytes_ptr, ctrl_byte_count(total_size));
				element_allocator_type::deallocate(element_ptr, total_size);
			}
			ctrl_bytes_ptr	= table_empty_group;
			element_ptr		= nullptr;
			tombstone_count = 0;
			current_size	= 0;
			total_size		= 0;
			num_groups		= 1;
		}

		~table_storage() noexcept {
			release();
		}
	};

}
