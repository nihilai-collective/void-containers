/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Nihilai Collective Corp
 * https://github.com/nihilai-collective/void-containers
 * include/void-containers-incl/containers/concepts.hpp
 */
#pragma once

#include <void-containers-incl/config.hpp>

namespace void_containers {

	template<typename value_type> using base_t = std::remove_cvref_t<value_type>;

	template<typename value_type_01, typename value_type_02>
	concept convertible_types = requires { static_cast<base_t<value_type_01>>(std::declval<base_t<value_type_02>>()); };

	template<typename value_type, typename key_type>
	concept lookup_key_types = std::same_as<base_t<value_type>, key_type> || requires { static_cast<key_type>(std::declval<const base_t<value_type>&>()); };

	template<typename value_type>
	concept integral_types = std::is_integral_v<base_t<value_type>> && !std::same_as<base_t<value_type>, bool>;

	template<typename value_type>
	concept uint_types = std::is_unsigned_v<base_t<value_type>> && integral_types<value_type>;

	template<typename value_type>
	concept uint64_types = uint_types<value_type> && sizeof(base_t<value_type>) == 8;

	template<typename value_type>
	concept size_types = requires {
		{ std::declval<base_t<value_type>>().size() } -> std::same_as<typename base_t<value_type>::size_type>;
	};

	template<typename value_type>
	concept data_fn_types = requires {
		{ std::declval<base_t<value_type>>().data() } -> std::same_as<typename base_t<value_type>::pointer>;
	} || requires {
		{ std::declval<base_t<value_type>>().data() } -> std::same_as<typename base_t<value_type>::const_pointer>;
	};

	template<typename value_type>
	concept find_types = requires {
		{ std::declval<base_t<value_type>>().find(std::declval<typename base_t<value_type>::value_type>()) } -> std::same_as<typename base_t<value_type>::size_type>;
	};

	template<typename value_type>
	concept vector_subscriptable_types = requires {
		{ std::declval<base_t<value_type>>()[std::declval<typename base_t<value_type>::size_type>()] } -> std::same_as<typename base_t<value_type>::reference>;
	} || requires {
		{ std::declval<base_t<value_type>>()[std::declval<typename base_t<value_type>::size_type>()] } -> std::same_as<typename base_t<value_type>::const_reference>;
	};

	template<typename value_type>
	concept string_types = vector_subscriptable_types<value_type> && data_fn_types<value_type> && size_types<value_type> && find_types<value_type>;

	template<typename value_type>
	concept pair_types = requires {
		typename base_t<value_type>::first_type;
		typename base_t<value_type>::second_type;
	};

}
