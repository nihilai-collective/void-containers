/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Nihilai Collective Corp
 * https://github.com/nihilai-collective/void-containers
 * include/void-containers-incl/containers/table_iterator.hpp
 */
#pragma once

#include <void-containers-incl/config.hpp>
#include <void-containers-incl/containers/table_control.hpp>

namespace void_containers {

	template<typename value_type_new> struct table_iterator {
		using value_type		= value_type_new;
		using iterator_category = std::forward_iterator_tag;
		using difference_type	= std::ptrdiff_t;
		using pointer			= value_type*;
		using reference			= value_type&;

		pointer element{ nullptr };
		const ctrl_byte_type* ctrl{ nullptr };

		VC_INLINE table_iterator() noexcept = default;

		VC_INLINE table_iterator(pointer element_new, const ctrl_byte_type* ctrl_new) noexcept : element{ element_new }, ctrl{ ctrl_new } {
		}

		template<typename other_type>
			requires(std::same_as<value_type, const other_type>)
		VC_INLINE table_iterator(const table_iterator<other_type>& other) noexcept : element{ other.element }, ctrl{ other.ctrl } {
		}

		VC_INLINE reference operator*() const noexcept {
			return *element;
		}

		VC_INLINE pointer operator->() const noexcept {
			return element;
		}

		VC_INLINE table_iterator& operator++() noexcept {
			++element;
			++ctrl;
			while (ctrl_constants::is_empty_or_deleted(*ctrl)) {
				++element;
				++ctrl;
			}
			if (ctrl_constants::is_sentinel(*ctrl)) {
				element = nullptr;
				ctrl	= nullptr;
			}
			return *this;
		}

		VC_INLINE table_iterator operator++(int) noexcept {
			table_iterator previous{ *this };
			++(*this);
			return previous;
		}

		VC_INLINE bool operator==(const table_iterator& other) const noexcept {
			return element == other.element;
		}
	};

}
