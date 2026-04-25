/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Nihilai Collective Corp
 * https://github.com/nihilai-collective/void-containers
 * include/void-containers-incl/containers/sonnet_map.hpp
 */
#pragma once

#include <void-containers-incl/config.hpp>
#include <void-containers-incl/containers/concepts.hpp>
#include <void-containers-incl/containers/hasher.hpp>
#include <void-containers-incl/containers/key_equal.hpp>
#include <void-containers-incl/containers/table_operations.hpp>

namespace void_containers {

	template<typename first_type_new, typename second_type_new> struct pair {
		using first_type  = first_type_new;
		using second_type = second_type_new;
		first_type first;
		second_type second;
	};

	template<typename key_type_new, typename mapped_type_new> struct map_slot_policy {
		using key_type		= key_type_new;
		using mapped_type	= mapped_type_new;
		using value_type	= pair<key_type, mapped_type>;
		using iterated_type = value_type;

		VC_INLINE static const key_type& key_of(const value_type& value) noexcept {
			return value.first;
		}

		VC_INLINE static bool values_equal(const value_type& lhs, const value_type& rhs) noexcept {
			return lhs.second == rhs.second;
		}

		VC_INLINE static uint64_t dynamic_bytes(const value_type& value) noexcept {
			uint64_t total{};
			if constexpr (size_types<key_type>) {
				total += value.first.size() * sizeof(typename key_type::value_type);
			}
			if constexpr (size_types<mapped_type>) {
				total += value.second.size() * sizeof(typename mapped_type::value_type);
			}
			return total;
		}
	};

	template<typename key_type_new, typename mapped_type_new, typename hasher_type = rt_key_hasher<key_type_new>, typename key_equal_type = rt_key_equal<key_type_new>>
	struct sonnet_map : public table_operations<map_slot_policy<key_type_new, mapped_type_new>, hasher<key_type_new, hasher_type>, key_equal_type> {
		using base				 = table_operations<map_slot_policy<key_type_new, mapped_type_new>, hasher<key_type_new, hasher_type>, key_equal_type>;
		using key_type			 = key_type_new;
		using mapped_type		 = mapped_type_new;
		using value_type		 = typename base::value_type;
		using size_type			 = uint64_t;
		using insert_result_type = typename base::insert_result_type;

		sonnet_map(const size_type size_new = 0) noexcept : base{ size_new } {
		}

		sonnet_map(std::initializer_list<value_type> values) noexcept : base{ 0 } {
			insert(values);
		}

		template<std::input_iterator iterator_type_new> sonnet_map(iterator_type_new first, iterator_type_new last) noexcept : base{ 0 } {
			insert(first, last);
		}

		template<lookup_key_types<key_type> key_type_newer = key_type> mapped_type& at(const key_type_newer& value) {
			const size_type index{ this->probe_find(value) };
			if (index == base::npos) {
				throw std::out_of_range{ "sonnet_map::at: key not found" };
			}
			return this->element_ptr[index].second;
		}

		template<lookup_key_types<key_type> key_type_newer = key_type> const mapped_type& at(const key_type_newer& value) const {
			const size_type index{ this->probe_find(value) };
			if (index == base::npos) {
				throw std::out_of_range{ "sonnet_map::at: key not found" };
			}
			return this->element_ptr[index].second;
		}

		template<typename key_type_newer = key_type> VC_INLINE mapped_type& operator[](key_type_newer&& value) noexcept {
			return try_emplace(std::forward<key_type_newer>(value)).first->second;
		}

		template<typename key_type_newer, typename... arg_types> VC_INLINE insert_result_type try_emplace(key_type_newer&& key, arg_types&&... args) noexcept {
			this->grow_if_needed();
			size_type index{};
			if (this->probe_find_for_insertion(key, index)) {
				return { this->iterator_at(index), false };
			}
			new (&this->element_ptr[index]) value_type{ static_cast<key_type>(std::forward<key_type_newer>(key)), mapped_type(std::forward<arg_types>(args)...) };
			return { this->iterator_at(index), true };
		}

		template<typename key_type_newer, typename mapped_type_newer> VC_INLINE insert_result_type insert_or_assign(key_type_newer&& key, mapped_type_newer&& value) noexcept {
			insert_result_type result{ try_emplace(std::forward<key_type_newer>(key), std::forward<mapped_type_newer>(value)) };
			if (!result.second) {
				result.first->second = std::forward<mapped_type_newer>(value);
			}
			return result;
		}

		template<typename key_type_newer, typename mapped_type_newer> VC_INLINE insert_result_type insert(key_type_newer&& key, mapped_type_newer&& value) noexcept {
			return try_emplace(std::forward<key_type_newer>(key), std::forward<mapped_type_newer>(value));
		}

		template<pair_types pair_type> VC_INLINE insert_result_type insert(pair_type&& value) noexcept {
			return insert(std::forward<pair_type>(value).first, std::forward<pair_type>(value).second);
		}

		template<std::input_iterator iterator_type_new>
			requires(pair_types<std::iter_value_t<iterator_type_new>>)
		void insert(iterator_type_new first, iterator_type_new last) noexcept {
			for (; first != last; ++first) {
				insert(*first);
			}
		}

		void insert(std::initializer_list<value_type> values) noexcept {
			insert(values.begin(), values.end());
		}
	};

}
