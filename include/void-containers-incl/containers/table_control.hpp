/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Nihilai Collective Corp
 * https://github.com/nihilai-collective/void-containers
 * include/void-containers-incl/containers/table_control.hpp
 */
#pragma once

#include <void-containers-incl/config.hpp>

namespace void_containers {

	using ctrl_byte_type = int8_t;

	struct ctrl_constants {
		static constexpr ctrl_byte_type deleted_val{ static_cast<ctrl_byte_type>(0b11111110) };
		static constexpr ctrl_byte_type empty_val{ static_cast<ctrl_byte_type>(0b10000000) };
		static constexpr ctrl_byte_type full_val{ static_cast<ctrl_byte_type>(0b00000000) };
		static constexpr ctrl_byte_type sentinel_val{ static_cast<ctrl_byte_type>(0b11111111) };

		VC_INLINE static constexpr bool is_empty_or_deleted(const ctrl_byte_type byte) noexcept {
			return byte < sentinel_val;
		}

		VC_INLINE static constexpr bool is_sentinel(const ctrl_byte_type byte) noexcept {
			return byte == sentinel_val;
		}

		VC_INLINE static constexpr bool is_full(const ctrl_byte_type byte) noexcept {
			return byte >= full_val;
		}
	};

	inline constexpr uint64_t table_group_width{ 16 };

	alignas(table_group_width) inline constinit ctrl_byte_type table_empty_group[table_group_width]{ ctrl_constants::empty_val, ctrl_constants::empty_val,
		ctrl_constants::empty_val, ctrl_constants::empty_val, ctrl_constants::empty_val, ctrl_constants::empty_val, ctrl_constants::empty_val, ctrl_constants::empty_val,
		ctrl_constants::empty_val, ctrl_constants::empty_val, ctrl_constants::empty_val, ctrl_constants::empty_val, ctrl_constants::empty_val, ctrl_constants::empty_val,
		ctrl_constants::empty_val, ctrl_constants::empty_val };

}
