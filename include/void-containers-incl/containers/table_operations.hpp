/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Nihilai Collective Corp
 * https://github.com/nihilai-collective/void-containers
 * include/void-containers-incl/containers/table_operations.hpp
 */
#pragma once

#include <void-containers-incl/config.hpp>
#include <void-containers-incl/containers/concepts.hpp>
#include <void-containers-incl/containers/table_iterator.hpp>
#include <void-containers-incl/containers/table_storage.hpp>
#include <void-containers-incl/simd/bit_ops.hpp>
#include <void-containers-incl/simd/compare.hpp>

namespace void_containers {

	template<typename slot_policy_new, typename hasher_type_new, typename key_equal_type_new>
	struct table_operations : public table_storage<typename slot_policy_new::value_type>, public hasher_type_new, public key_equal_type_new {
		using slot_policy		  = slot_policy_new;
		using base				  = table_storage<typename slot_policy::value_type>;
		using size_type			  = uint64_t;
		using value_type		  = typename base::value_type;
		using key_type			  = typename slot_policy::key_type;
		using hasher_type		  = hasher_type_new;
		using key_equal_type	  = key_equal_type_new;
		using simd_type			  = internal::simd_int_128_t;
		using iterator_type		  = table_iterator<typename slot_policy::iterated_type>;
		using const_iterator_type = table_iterator<const value_type>;
		using insert_result_type  = std::pair<iterator_type, bool>;

		static constexpr size_type npos{ std::numeric_limits<size_type>::max() };

		table_operations(const size_type size_new) noexcept : base{ size_new } {
		}

		size_type total_memory_usage() const noexcept {
			size_type total{ sizeof(*this) };
			if (base::total_size != 0) {
				total += base::ctrl_byte_count(base::total_size) * sizeof(ctrl_byte_type) + base::total_size * sizeof(value_type);
				for (size_type index = 0; index < base::total_size; ++index) {
					if (ctrl_constants::is_full(base::ctrl_bytes_ptr[index])) {
						total += slot_policy::dynamic_bytes(base::element_ptr[index]);
					}
				}
			}
			return total;
		}

		VC_INLINE iterator_type begin() noexcept {
			const size_type index{ first_full_index() };
			return index == npos ? iterator_type{} : iterator_at(index);
		}

		VC_INLINE iterator_type end() noexcept {
			return iterator_type{};
		}

		VC_INLINE const_iterator_type begin() const noexcept {
			const size_type index{ first_full_index() };
			return index == npos ? const_iterator_type{} : const_iterator_type{ base::element_ptr + index, base::ctrl_bytes_ptr + index };
		}

		VC_INLINE const_iterator_type end() const noexcept {
			return const_iterator_type{};
		}

		VC_INLINE const_iterator_type cbegin() const noexcept {
			return begin();
		}

		VC_INLINE const_iterator_type cend() const noexcept {
			return end();
		}

		void clear() noexcept {
			base::destroy_elements();
			std::fill(base::ctrl_bytes_ptr, base::ctrl_bytes_ptr + base::total_size, ctrl_constants::empty_val);
			base::current_size	  = 0;
			base::tombstone_count = 0;
		}

		void swap(table_operations& other) noexcept {
			base::swap_storage(other);
			std::swap(static_cast<hasher_type&>(*this), static_cast<hasher_type&>(other));
			std::swap(static_cast<key_equal_type&>(*this), static_cast<key_equal_type&>(other));
		}

		void reserve(const size_type count_new) noexcept {
			if (count_new != 0 && count_new >= base::growth_limit()) {
				rehash(count_new);
			}
		}

		template<lookup_key_types<key_type> key_type_newer = key_type> VC_INLINE iterator_type find(const key_type_newer& value) noexcept {
			const size_type index{ probe_find(value) };
			return index == npos ? iterator_type{} : iterator_at(index);
		}

		template<lookup_key_types<key_type> key_type_newer = key_type> VC_INLINE const_iterator_type find(const key_type_newer& value) const noexcept {
			const size_type index{ probe_find(value) };
			return index == npos ? const_iterator_type{} : const_iterator_type{ base::element_ptr + index, base::ctrl_bytes_ptr + index };
		}

		template<lookup_key_types<key_type> key_type_newer = key_type> VC_INLINE bool contains(const key_type_newer& value) const noexcept {
			return probe_find(value) != npos;
		}

		template<lookup_key_types<key_type> key_type_newer = key_type> VC_INLINE size_type count(const key_type_newer& value) const noexcept {
			return contains(value) ? 1ULL : 0ULL;
		}

		template<lookup_key_types<key_type> key_type_newer = key_type> VC_INLINE void prefetch(const key_type_newer& value) const noexcept {
			const uint64_t hash_value{ hasher_type::operator()(lookup_key(value)) };
			const size_type group_index{ hasher_type::h1(hash_value) & (base::num_groups - 1ULL) };
			internal::prefetch(base::ctrl_bytes_ptr + group_index * table_group_width);
		}

		template<typename... arg_types> VC_INLINE insert_result_type emplace(arg_types&&... args) noexcept {
			value_type value(std::forward<arg_types>(args)...);
			grow_if_needed();
			size_type index{};
			if (probe_find_for_insertion(slot_policy::key_of(value), index)) {
				return { iterator_at(index), false };
			}
			new (&base::element_ptr[index]) value_type(std::move(value));
			return { iterator_at(index), true };
		}

		VC_INLINE bool erase(iterator_type value) noexcept {
			return erase_iterator(value.element);
		}

		VC_INLINE bool erase(const_iterator_type value) noexcept
			requires(!std::same_as<iterator_type, const_iterator_type>)
		{
			return erase_iterator(value.element);
		}

		VC_INLINE iterator_type erase(iterator_type first, const iterator_type last) noexcept {
			while (first != last) {
				const iterator_type current{ first };
				++first;
				erase_iterator(current.element);
			}
			return last;
		}

		template<lookup_key_types<key_type> key_type_newer> VC_INLINE bool erase(const key_type_newer& value) noexcept {
			const size_type index{ probe_find(value) };
			if (index == npos) {
				return false;
			}
			erase_slot(index);
			return true;
		}

		bool operator==(const table_operations& other) const noexcept {
			if (base::current_size != other.current_size) {
				return false;
			}
			for (size_type index = 0; index < base::total_size; ++index) {
				if (ctrl_constants::is_full(base::ctrl_bytes_ptr[index])) {
					const size_type other_index{ other.probe_find(slot_policy::key_of(base::element_ptr[index])) };
					if (other_index == npos || !slot_policy::values_equal(base::element_ptr[index], other.element_ptr[other_index])) {
						return false;
					}
				}
			}
			return true;
		}

		void rehash(size_type new_capacity) noexcept {
			new_capacity = base::get_new_size(std::max(new_capacity, base::current_size));

			ctrl_byte_type* old_ctrl{ base::ctrl_bytes_ptr };
			value_type* old_elements{ base::element_ptr };
			const size_type old_capacity{ base::total_size };

			base::allocate_table(new_capacity);

			if (old_capacity != 0) {
				for (size_type index = 0; index < old_capacity; ++index) {
					if (ctrl_constants::is_full(old_ctrl[index])) {
						size_type new_index{};
						probe_find_for_insertion(slot_policy::key_of(old_elements[index]), new_index);
						new (&base::element_ptr[new_index]) value_type(std::move(old_elements[index]));
						old_elements[index].~value_type();
					}
				}
				base::ctrl_byte_allocator_type::deallocate(old_ctrl, base::ctrl_byte_count(old_capacity));
				base::element_allocator_type::deallocate(old_elements, old_capacity);
			}
		}

	  protected:
		template<typename key_type_newer> VC_INLINE static decltype(auto) lookup_key(const key_type_newer& value) noexcept {
			if constexpr (string_types<key_type> && !string_types<key_type_newer>) {
				return std::basic_string_view<typename key_type::value_type>(value);
			} else {
				return (value);
			}
		}

		VC_INLINE iterator_type iterator_at(const size_type index) noexcept {
			return iterator_type{ base::element_ptr + index, base::ctrl_bytes_ptr + index };
		}

		VC_INLINE void grow_if_needed() noexcept {
			if (base::should_resize()) {
				rehash(base::current_size + 1ULL);
			}
		}

		VC_INLINE size_type first_full_index() const noexcept {
			if (base::current_size == 0) {
				return npos;
			}
			size_type index{};
			while (ctrl_constants::is_empty_or_deleted(base::ctrl_bytes_ptr[index])) {
				++index;
			}
			return index;
		}

		template<typename key_type_newer> VC_INLINE bool key_matches(const size_type index, const key_type_newer& value) const noexcept {
			const key_type& key{ slot_policy::key_of(base::element_ptr[index]) };
			const key_equal_type& equal{ *this };
			if constexpr (requires { equal(key, value); }) {
				return equal(key, value);
			} else {
				return equal(key, key_type(value));
			}
		}

		VC_INLINE bool erase_iterator(const value_type* element) noexcept {
			if (element == nullptr) {
				return false;
			}
			const size_type index{ static_cast<size_type>(element - base::element_ptr) };
			if (index >= base::total_size || !ctrl_constants::is_full(base::ctrl_bytes_ptr[index])) {
				return false;
			}
			erase_slot(index);
			return true;
		}

		VC_INLINE void erase_slot(const size_type index) noexcept {
			base::element_ptr[index].~value_type();
			base::ctrl_bytes_ptr[index] = ctrl_constants::deleted_val;
			--base::current_size;
			++base::tombstone_count;
		}

		VC_INLINE void claim_slot(const size_type index, const ctrl_byte_type h2_value) noexcept {
			base::ctrl_bytes_ptr[index] = h2_value;
			++base::current_size;
		}

		template<lookup_key_types<key_type> key_type_newer = key_type> VC_INLINE bool probe_find_for_insertion(const key_type_newer& value_new, size_type& index_new) noexcept {
			const auto& value{ lookup_key(value_new) };
			const uint64_t hash_value{ hasher_type::operator()(value) };
			const ctrl_byte_type h2_value{ hasher_type::h2(hash_value) };
			const auto h2_simd_value{ internal::op_set1<simd_type>(static_cast<uint8_t>(h2_value)) };
			const auto empty_simd_value{ internal::op_set1<simd_type>(static_cast<uint8_t>(ctrl_constants::empty_val)) };
			const auto deleted_simd_value{ internal::op_set1<simd_type>(static_cast<uint8_t>(ctrl_constants::deleted_val)) };

			size_type group_index{ hasher_type::h1(hash_value) & (base::num_groups - 1ULL) };
			size_type first_tombstone_index{ npos };

			while (true) {
				const size_type group_offset{ group_index * table_group_width };
				const auto bytes{ internal::load_values<simd_type>(base::ctrl_bytes_ptr + group_offset) };

				auto matches{ internal::op_cmp_eq<simd_type>(h2_simd_value, bytes) };
				while (matches != 0) {
					const size_type match_index{ group_offset + internal::tzcnt(matches) };
					if (key_matches(match_index, value)) {
						index_new = match_index;
						return true;
					}
					matches = internal::blsr(matches);
				}

				if (base::tombstone_count > 0 && first_tombstone_index == npos) {
					const auto deleted_mask{ internal::op_cmp_eq<simd_type>(bytes, deleted_simd_value) };
					if (deleted_mask != 0) {
						first_tombstone_index = group_offset + internal::tzcnt(deleted_mask);
					}
				}

				const auto empty_matches{ internal::op_cmp_eq<simd_type>(bytes, empty_simd_value) };
				if (empty_matches != 0) {
					if (first_tombstone_index != npos) {
						index_new = first_tombstone_index;
						--base::tombstone_count;
					} else {
						index_new = group_offset + internal::tzcnt(empty_matches);
					}
					claim_slot(index_new, h2_value);
					return false;
				}

				group_index = (group_index + 1ULL) & (base::num_groups - 1ULL);
			}
		}

		template<lookup_key_types<key_type> key_type_newer = key_type> VC_INLINE size_type probe_find(const key_type_newer& value_new) const noexcept {
			const auto& value{ lookup_key(value_new) };
			const uint64_t hash_value{ hasher_type::operator()(value) };
			const auto h2_simd_value{ internal::op_set1<simd_type>(static_cast<uint8_t>(hasher_type::h2(hash_value))) };
			const auto empty_simd_value{ internal::op_set1<simd_type>(static_cast<uint8_t>(ctrl_constants::empty_val)) };

			size_type group_index{ hasher_type::h1(hash_value) & (base::num_groups - 1ULL) };

			while (true) {
				const size_type group_offset{ group_index * table_group_width };
				const auto bytes{ internal::load_values<simd_type>(base::ctrl_bytes_ptr + group_offset) };

				auto matches{ internal::op_cmp_eq<simd_type>(h2_simd_value, bytes) };
				while (matches != 0) {
					const size_type match_index{ group_offset + internal::tzcnt(matches) };
					if (key_matches(match_index, value)) {
						return match_index;
					}
					matches = internal::blsr(matches);
				}

				if (internal::op_cmp_eq<simd_type>(empty_simd_value, bytes) != 0) {
					return npos;
				}

				group_index = (group_index + 1ULL) & (base::num_groups - 1ULL);
			}
		}
	};

	template<typename container_type, typename predicate_type> uint64_t erase_if(container_type& container, predicate_type predicate) {
		uint64_t erased{};
		auto current	= container.begin();
		const auto last = container.end();
		while (current != last) {
			const auto candidate = current;
			++current;
			if (predicate(*candidate)) {
				container.erase(candidate);
				++erased;
			}
		}
		return erased;
	}

}
