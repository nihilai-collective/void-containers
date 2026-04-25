/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Nihilai Collective Corp
 * https://github.com/nihilai-collective/void-containers
 * include/void-containers-incl/containers/key_equal.hpp
 */
#pragma once

#include <void-containers-incl/config.hpp>
#include <void-containers-incl/containers/concepts.hpp>
#include <void-containers-incl/simd/compare.hpp>

namespace void_containers {

	template<typename key_type> struct rt_key_equal {
		template<typename lhs_type, typename rhs_type> VC_INLINE bool operator()(const lhs_type& lhs, const rhs_type& rhs) const noexcept {
			if constexpr (string_types<key_type>) {
				return lhs.size() == rhs.size() && internal::comparison::compare(lhs.data(), rhs.data(), lhs.size());
			} else if constexpr (std::same_as<lhs_type, rhs_type>) {
				return lhs == rhs;
			} else {
				return lhs == static_cast<key_type>(rhs);
			}
		}
	};

}
