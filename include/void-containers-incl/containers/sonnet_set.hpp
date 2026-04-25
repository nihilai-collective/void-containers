/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Nihilai Collective Corp
 * https://github.com/nihilai-collective/void-containers
 * include/void-containers-incl/containers/sonnet_set.hpp
 */
#pragma once

#include <void-containers-incl/config.hpp>
#include <void-containers-incl/containers/concepts.hpp>
#include <void-containers-incl/containers/hasher.hpp>
#include <void-containers-incl/containers/key_equal.hpp>
#include <void-containers-incl/containers/table_operations.hpp>

namespace void_containers {

	template<typename key_type_new> struct set_slot_policy {
		using key_type		= key_type_new;
		using value_type	= key_type;
		using iterated_type = const value_type;

		VC_INLINE static const key_type& key_of(const value_type& value) noexcept {
			return value;
		}

		VC_INLINE static bool values_equal(const value_type&, const value_type&) noexcept {
			return true;
		}

		VC_INLINE static uint64_t dynamic_bytes([[maybe_unused]] const value_type& value) noexcept {
			if constexpr (size_types<key_type>) {
				return value.size() * sizeof(typename key_type::value_type);
			} else {
				return 0;
			}
		}
	};

	template<typename key_type_new, typename hasher_type = rt_key_hasher<key_type_new>, typename key_equal_type = rt_key_equal<key_type_new>> struct sonnet_set
		: public table_operations<set_slot_policy<key_type_new>, hasher<key_type_new, hasher_type>, key_equal_type> {
		using base				 = table_operations<set_slot_policy<key_type_new>, hasher<key_type_new, hasher_type>, key_equal_type>;
		using key_type			 = key_type_new;
		using value_type		 = typename base::value_type;
		using size_type			 = uint64_t;
		using insert_result_type = typename base::insert_result_type;

		sonnet_set(const size_type size_new = 0) noexcept : base{ size_new } {
		}

		sonnet_set(std::initializer_list<value_type> values) noexcept : base{ 0 } {
			insert(values);
		}

		template<std::input_iterator iterator_type_new> sonnet_set(iterator_type_new first, iterator_type_new last) noexcept : base{ 0 } {
			insert(first, last);
		}

		template<typename key_type_newer> VC_INLINE insert_result_type insert(key_type_newer&& key) noexcept {
			this->grow_if_needed();
			size_type index{};
			if (this->probe_find_for_insertion(key, index)) {
				return { this->iterator_at(index), false };
			}
			new (&this->element_ptr[index]) value_type(static_cast<key_type>(std::forward<key_type_newer>(key)));
			return { this->iterator_at(index), true };
		}

		template<std::input_iterator iterator_type_new> void insert(iterator_type_new first, iterator_type_new last) noexcept {
			for (; first != last; ++first) {
				insert(*first);
			}
		}

		void insert(std::initializer_list<value_type> values) noexcept {
			insert(values.begin(), values.end());
		}
	};

}
