/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Nihilai Collective Corp
 * https://github.com/nihilai-collective/void-containers
 * unit-tests/sonnet_set_tests.cpp
 */

#include <unordered_set>
#include <random>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <memory>
#include <vector>
#include <iostream>
#include <rt-ut>
#include <void-containers>

using namespace void_containers;
using namespace rt_ut;

using int32	 = int32_t;
using int64	 = int64_t;
using uint32 = uint32_t;
using uint64 = uint64_t;

int32 run_sonnet_set_tests();

namespace {

	struct Tracked {
		inline static int64 alive{};
		uint32 value{};

		Tracked(const uint32 value_new) : value{ value_new } {
			++alive;
		}
		Tracked(const Tracked& other) : value{ other.value } {
			++alive;
		}
		Tracked(Tracked&& other) noexcept : value{ other.value } {
			++alive;
		}
		Tracked& operator=(const Tracked&) = default;
		~Tracked() {
			--alive;
		}
		bool operator==(const Tracked& other) const {
			return value == other.value;
		}
	};

	struct parity_hasher {
		uint64 operator()(const uint32 key) const noexcept {
			return fmix64(key % 2U);
		}
	};

	struct parity_equal {
		bool operator()(const uint32 lhs, const uint32 rhs) const noexcept {
			return lhs % 2U == rhs % 2U;
		}
	};

	struct move_only_key {
		std::unique_ptr<uint32> value;

		explicit move_only_key(std::unique_ptr<uint32>&& value_new) noexcept : value{ std::move(value_new) } {
		}
		move_only_key(const move_only_key&)				   = delete;
		move_only_key& operator=(const move_only_key&)	   = delete;
		move_only_key(move_only_key&&) noexcept			   = default;
		move_only_key& operator=(move_only_key&&) noexcept = delete;
		~move_only_key()								   = default;

		bool operator==(const move_only_key& other) const {
			return *value == *other.value;
		}
	};

	struct move_only_hasher {
		uint64 operator()(const move_only_key& key) const noexcept {
			return fmix64(*key.value);
		}
	};

	struct tracked_hasher {
		uint64 operator()(const Tracked& key) const noexcept {
			return fmix64(key.value);
		}
	};

}

int32 run_sonnet_set_tests() {
	uint64 failure_count{};
	auto tally = [&](bool result) {
		if (!result) {
			++failure_count;
		}
	};

	tally(unit_test<"sonnet_set: fresh set is empty", true, false>::assert_eq(true, [] {
		sonnet_set<uint32> set{};
		return set.size() == 0 && set.capacity() == 0 && set.begin() == set.end() && !set.contains(1U) && set.find(1U) == set.end();
	}));

	tally(unit_test<"sonnet_set: insert reports whether it added", true, false>::assert_eq(true, [] {
		sonnet_set<uint32> set{};
		const bool first{ set.insert(7U).second };
		const bool second{ set.insert(7U).second };
		return first && !second && set.size() == 1 && set.contains(7U);
	}));

	tally(unit_test<"sonnet_set: find returns the stored key", true, false>::assert_eq(true, [] {
		sonnet_set<uint32> set{};
		set.insert(42U);
		auto found = set.find(42U);
		return found != set.end() && *found == 42U && set.find(43U) == set.end();
	}));

	tally(unit_test<"sonnet_set: iterated keys are immutable", true, false>::assert_eq(true, [] {
		using reference = decltype(*std::declval<sonnet_set<uint32>&>().begin());
		return std::is_const_v<std::remove_reference_t<reference>>;
	}));

	tally(unit_test<"sonnet_set: erase by key reports presence", true, false>::assert_eq(true, [] {
		sonnet_set<uint32> set{};
		set.insert(1U);
		set.insert(2U);
		const bool first{ set.erase(1U) };
		const bool second{ set.erase(1U) };
		return first && !second && set.size() == 1 && !set.contains(1U) && set.contains(2U);
	}));

	tally(unit_test<"sonnet_set: erase by iterator removes the key", true, false>::assert_eq(true, [] {
		sonnet_set<uint32> set{};
		set.insert(5U);
		auto found = set.find(5U);
		const bool erased{ set.erase(found) };
		auto missing = set.end();
		return erased && !set.erase(missing) && set.size() == 0 && !set.contains(5U);
	}));

	tally(unit_test<"sonnet_set: erased key can return", true, false>::assert_eq(true, [] {
		sonnet_set<uint32> set{};
		set.insert(9U);
		set.erase(9U);
		return set.insert(9U).second && set.size() == 1 && set.contains(9U);
	}));

	tally(unit_test<"sonnet_set: growth retains every key", true, false>::assert_eq(true, [] {
		sonnet_set<uint32> set{};
		for (uint32 key = 0; key < 10000; ++key) {
			if (!set.insert(key).second) {
				return false;
			}
		}
		for (uint32 key = 0; key < 10000; ++key) {
			if (!set.contains(key)) {
				return false;
			}
		}
		return set.size() == 10000 && !set.contains(10000U);
	}));

	tally(unit_test<"sonnet_set: iteration visits each key once", true, false>::assert_eq(true, [] {
		sonnet_set<uint32> set{};
		for (uint32 key = 0; key < 1000; ++key) {
			set.insert(key);
		}
		for (uint32 key = 0; key < 1000; key += 2U) {
			set.erase(key);
		}
		std::unordered_set<uint32> seen{};
		uint64 visits{};
		for (const uint32 key: set) {
			++visits;
			if (key % 2U == 0U || !seen.insert(key).second) {
				return false;
			}
		}
		return visits == 500 && seen.size() == 500;
	}));

	tally(unit_test<"sonnet_set: const set supports lookup and iteration", true, false>::assert_eq(true, [] {
		sonnet_set<uint32> set{};
		set.insert(3U);
		const sonnet_set<uint32>& view{ set };
		uint64 visits{};
		for (const uint32 key: view) {
			visits += key == 3U ? 1ULL : 0ULL;
		}
		return visits == 1 && view.contains(3U) && view.find(4U) == view.end() && view.size() == 1;
	}));

	tally(unit_test<"sonnet_set: clear empties and stays usable", true, false>::assert_eq(true, [] {
		sonnet_set<uint32> set{};
		for (uint32 key = 0; key < 100; ++key) {
			set.insert(key);
		}
		set.clear();
		const bool emptied{ set.size() == 0 && set.begin() == set.end() && !set.contains(1U) };
		return emptied && set.insert(1U).second && set.size() == 1;
	}));

	tally(unit_test<"sonnet_set: copy is independent and can grow", true, false>::assert_eq(true, [] {
		sonnet_set<uint32> source{};
		for (uint32 key = 0; key < 100; ++key) {
			source.insert(key);
		}
		sonnet_set<uint32> copy{};
		copy = source;
		copy.erase(1U);
		for (uint32 key = 100; key < 5000; ++key) {
			copy.insert(key);
		}
		return source.size() == 100 && source.contains(1U) && !source.contains(100U) && copy.size() == 4999 && !copy.contains(1U) && copy.contains(4999U);
	}));

	tally(unit_test<"sonnet_set: move transfers ownership", true, false>::assert_eq(true, [] {
		sonnet_set<uint32> source{};
		source.insert(1U);
		sonnet_set<uint32> constructed{ std::move(source) };
		sonnet_set<uint32> assigned{};
		assigned.insert(99U);
		assigned = std::move(constructed);
		return assigned.size() == 1 && assigned.contains(1U) && !assigned.contains(99U) && constructed.size() == 0 && constructed.capacity() == 0;
	}));

	tally(unit_test<"sonnet_set: rehash preserves keys both ways", true, false>::assert_eq(true, [] {
		sonnet_set<uint32> set{};
		for (uint32 key = 0; key < 1000; ++key) {
			set.insert(key);
		}
		set.rehash(1ULL << 14U);
		const bool grew{ set.capacity() >= (1ULL << 14U) };
		set.rehash(1);
		for (uint32 key = 0; key < 1000; ++key) {
			if (!set.contains(key)) {
				return false;
			}
		}
		return grew && set.size() == 1000;
	}));

	tally(unit_test<"sonnet_set: string keys of every length work", true, false>::assert_eq(true, [] {
		sonnet_set<std::string> set{};
		for (uint32 length = 0; length < 70; ++length) {
			if (!set.insert(std::string(length, 'k')).second) {
				return false;
			}
		}
		for (uint32 length = 0; length < 70; ++length) {
			const std::string key(length, 'k');
			if (!set.contains(key) || set.insert(key).second) {
				return false;
			}
		}
		return set.size() == 70 && !set.contains(std::string(70, 'k')) && !set.contains(std::string(16, 'j'));
	}));

	tally(unit_test<"sonnet_set: string_view finds without a string", true, false>::assert_eq(true, [] {
		sonnet_set<std::string> set{};
		set.insert(std::string{ "a key long enough to live on the heap" });
		const std::string_view present{ "a key long enough to live on the heap" };
		const std::string_view absent{ "a key long enough to live on the hea?" };
		return set.contains(present) && !set.contains(absent);
	}));

	tally(unit_test<"sonnet_set: churn matches std::unordered_set", true, false>::assert_eq(true, [] {
		sonnet_set<uint32> set{};
		std::unordered_set<uint32> oracle{};
		std::mt19937 generator{ 12345U };
		for (uint32 step = 0; step < 200000; ++step) {
			const uint32 key{ static_cast<uint32>(generator()) % 4096U };
			if (generator() % 2U == 0U) {
				if (set.insert(key).second != oracle.insert(key).second) {
					return false;
				}
			} else if (set.erase(key) != (oracle.erase(key) == 1U)) {
				return false;
			}
		}
		for (uint32 key = 0; key < 4096; ++key) {
			if (set.contains(key) != oracle.contains(key)) {
				return false;
			}
		}
		return set.size() == oracle.size();
	}));

	tally(unit_test<"sonnet_set: RAII lifetime tracking on insert, erase, copy, clear, and destruction", true, false>::assert_eq(true, [] {
		Tracked::alive = 0;
		{
			sonnet_set<Tracked, tracked_hasher> set{};
			for (uint32 key = 0; key < 500; ++key) {
				set.insert(Tracked{ key });
			}
			if (Tracked::alive != 500) {
				return false;
			}
			for (uint32 key = 0; key < 100; ++key) {
				set.erase(Tracked{ key });
			}
			if (Tracked::alive != 400) {
				return false;
			}
			sonnet_set<Tracked, tracked_hasher> copy{};
			copy = set;
			if (Tracked::alive != 800) {
				return false;
			}
			copy.clear();
			if (Tracked::alive != 400) {
				return false;
			}
		}
		return Tracked::alive == 0;
	}));

	tally(unit_test<"sonnet_set: prefetch changes nothing", true, false>::assert_eq(true, [] {
		sonnet_set<uint32> set{};
		set.insert(1U);
		set.prefetch(1U);
		set.prefetch(2U);
		return set.size() == 1 && set.contains(1U) && !set.contains(2U);
	}));

	tally(unit_test<"sonnet_set: memory usage counts string contents", true, false>::assert_eq(true, [] {
		sonnet_set<std::string> set(16);
		const uint64 empty{ set.total_memory_usage() };
		set.insert(std::string(100, 'm'));
		return set.total_memory_usage() == empty + 100U;
	}));

	tally(unit_test<"sonnet_set: default construction does not allocate", true, false>::assert_eq(true, [] {
		sonnet_set<uint32> set{};
		return set.capacity() == 0 && set.element_ptr == nullptr && set.empty();
	}));

	tally(unit_test<"sonnet_set: empty set answers every query", true, false>::assert_eq(true, [] {
		sonnet_set<uint32> set{};
		set.prefetch(1U);
		set.clear();
		return !set.contains(1U) && set.count(1U) == 0 && !set.erase(1U) && set.cbegin() == set.cend();
	}));

	tally(unit_test<"sonnet_set: moved from set stays usable", true, false>::assert_eq(true, [] {
		sonnet_set<uint32> source{};
		source.insert(1U);
		sonnet_set<uint32> target{ std::move(source) };
		const bool queried{ !source.contains(1U) && source.empty() };
		return queried && source.insert(2U).second && source.contains(2U) && target.contains(1U) && !target.contains(2U);
	}));

	tally(unit_test<"sonnet_set: reserve prevents growth", true, false>::assert_eq(true, [] {
		sonnet_set<uint32> set{};
		set.reserve(1000);
		const uint64 reserved{ set.capacity() };
		for (uint32 key = 0; key < 1000; ++key) {
			set.insert(key);
		}
		return reserved >= 1000 && set.capacity() == reserved && set.size() == 1000;
	}));

	tally(unit_test<"sonnet_set: reserve zero does not allocate", true, false>::assert_eq(true, [] {
		sonnet_set<uint32> set{};
		set.reserve(0);
		return set.capacity() == 0;
	}));

	tally(unit_test<"sonnet_set: reserve keeps existing keys", true, false>::assert_eq(true, [] {
		sonnet_set<uint32> set{};
		set.insert(5U);
		set.reserve(5000);
		return set.contains(5U) && set.size() == 1;
	}));

	tally(unit_test<"sonnet_set: count reports presence", true, false>::assert_eq(true, [] {
		sonnet_set<uint32> set{};
		set.insert(1U);
		return set.count(1U) == 1 && set.count(2U) == 0;
	}));

	tally(unit_test<"sonnet_set: insert returns an iterator to the key", true, false>::assert_eq(true, [] {
		sonnet_set<uint32> set{};
		auto [inserted, added]		 = set.insert(8U);
		auto [existing, added_again] = set.insert(8U);
		return added && !added_again && *inserted == 8U && inserted == existing;
	}));

	tally(unit_test<"sonnet_set: emplace builds and deduplicates", true, false>::assert_eq(true, [] {
		sonnet_set<std::string> set{};
		const auto first  = set.emplace(3U, 'x');
		const auto second = set.emplace("xxx");
		return first.second && !second.second && *first.first == "xxx" && set.size() == 1;
	}));

	tally(unit_test<"sonnet_set: erase accepts a temporary iterator", true, false>::assert_eq(true, [] {
		sonnet_set<uint32> set{};
		set.insert(1U);
		return set.erase(set.find(1U)) && set.empty();
	}));

	tally(unit_test<"sonnet_set: erase range removes everything between", true, false>::assert_eq(true, [] {
		sonnet_set<uint32> set{};
		for (uint32 key = 0; key < 100; ++key) {
			set.insert(key);
		}
		const auto last = set.erase(set.begin(), set.end());
		return last == set.end() && set.empty() && !set.contains(1U);
	}));

	tally(unit_test<"sonnet_set: erase_if removes only matches", true, false>::assert_eq(true, [] {
		sonnet_set<uint32> set{};
		for (uint32 key = 0; key < 100; ++key) {
			set.insert(key);
		}
		const uint64 erased{ void_containers::erase_if(set, [](const uint32 key) {
			return key % 2U == 0U;
		}) };
		return erased == 50 && set.size() == 50 && set.contains(1U) && !set.contains(2U);
	}));

	tally(unit_test<"sonnet_set: swap exchanges contents", true, false>::assert_eq(true, [] {
		sonnet_set<uint32> first{};
		sonnet_set<uint32> second{};
		first.insert(1U);
		second.insert(2U);
		second.insert(3U);
		first.swap(second);
		return first.size() == 2 && first.contains(2U) && second.size() == 1 && second.contains(1U);
	}));

	tally(unit_test<"sonnet_set: initializer list deduplicates", true, false>::assert_eq(true, [] {
		sonnet_set<uint32> set{ 1U, 2U, 2U, 3U };
		return set.size() == 3 && set.contains(3U);
	}));

	tally(unit_test<"sonnet_set: range construction and insertion", true, false>::assert_eq(true, [] {
		const std::vector<uint32> keys{ 1U, 2U, 3U };
		sonnet_set<uint32> set(keys.begin(), keys.end());
		const std::vector<uint32> more{ 3U, 4U };
		set.insert(more.begin(), more.end());
		set.insert({ 5U, 6U });
		return set.size() == 6 && set.contains(1U) && set.contains(6U);
	}));

	tally(unit_test<"sonnet_set: equality ignores layout", true, false>::assert_eq(true, [] {
		sonnet_set<uint32> first{};
		sonnet_set<uint32> second(4096);
		for (uint32 key = 0; key < 100; ++key) {
			first.insert(key);
			second.insert(99U - key);
		}
		const bool equal{ first == second };
		second.erase(1U);
		second.insert(1000U);
		return equal && first != second;
	}));

	tally(unit_test<"sonnet_set: iterator converts to const iterator", true, false>::assert_eq(true, [] {
		sonnet_set<uint32> set{};
		set.insert(1U);
		sonnet_set<uint32>::const_iterator_type found = set.find(1U);
		return found != set.cend() && *found == 1U;
	}));

	tally(unit_test<"sonnet_set: string literals look up directly", true, false>::assert_eq(true, [] {
		sonnet_set<std::string> set{};
		set.insert("present");
		const char* pointer{ "present" };
		return set.contains("present") && set.contains(pointer) && !set.contains("absent") && set.erase("present") && set.empty();
	}));

	tally(unit_test<"sonnet_set: custom equality groups keys", true, false>::assert_eq(true, [] {
		sonnet_set<uint32, parity_hasher, parity_equal> set{};
		const bool first{ set.insert(1U).second };
		const bool same_parity{ set.insert(3U).second };
		const bool other_parity{ set.insert(2U).second };
		return first && !same_parity && other_parity && set.size() == 2 && set.contains(5U);
	}));

	tally(unit_test<"sonnet_set: move only keys survive growth", true, false>::assert_eq(true, [] {
		sonnet_set<move_only_key, move_only_hasher> set{};
		for (uint32 key = 0; key < 1000; ++key) {
			set.insert(move_only_key{ std::make_unique<uint32>(key) });
		}
		const move_only_key probe{ std::make_unique<uint32>(999U) };
		const move_only_key missing{ std::make_unique<uint32>(1000U) };
		return set.size() == 1000 && set.contains(probe) && !set.contains(missing);
	}));

	std::cout << "==== sonnet_set suite: " << (failure_count == 0 ? "all tests passed" : "failures detected") << " (" << failure_count << " failing) ====" << std::endl;

	return failure_count == 0 ? 0 : 1;
}
