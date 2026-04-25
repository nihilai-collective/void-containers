/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Nihilai Collective Corp
 * https://github.com/nihilai-collective/void-containers
 * unit-tests/sonnet_map_tests.cpp
 */

#include <bitset>
#include <unordered_map>
#include <random>
#include <string>
#include <vector>
#include <memory>
#include <iostream>
#include <stdexcept>
#include <cctype>
#include <string_view>
#include <rt-ut>
#include <void-containers>

using namespace void_containers;
using namespace void_containers::internal;
using namespace rt_ut;

using int8	  = int8_t;
using int32	  = int32_t;
using uint8	  = uint8_t;
using uint32  = uint32_t;
using uint64  = uint64_t;
using nothing = void;

struct Tracked {
	static uint32 alive;
	uint32 value;
	Tracked(uint32 v = 0) : value(v) {
		++alive;
	}
	Tracked(const Tracked& other) : value(other.value) {
		++alive;
	}
	Tracked(Tracked&& other) noexcept : value(other.value) {
		other.value = 0;
		++alive;
	}
	Tracked& operator=(const Tracked& other) {
		if (this != &other) {
			value = other.value;
		}
		return *this;
	}
	Tracked& operator=(Tracked&& other) noexcept {
		if (this != &other) {
			value		= other.value;
			other.value = 0;
		}
		return *this;
	}
	~Tracked() {
		--alive;
	}
	bool operator==(const Tracked&) const = default;
};
uint32 Tracked::alive = 0;

struct case_blind_hasher {
	uint64 operator()(const std::string& key) const noexcept {
		uint64 hash = 14695981039346656037ULL;
		for (const char value: key) {
			hash = (hash ^ static_cast<uint64>(std::tolower(static_cast<unsigned char>(value)))) * 1099511628211ULL;
		}
		return fmix64(hash);
	}
};

struct case_blind_equal {
	bool operator()(const std::string& lhs, const std::string& rhs) const noexcept {
		if (lhs.size() != rhs.size()) {
			return false;
		}
		for (uint64 i = 0; i < lhs.size(); ++i) {
			if (std::tolower(static_cast<unsigned char>(lhs[i])) != std::tolower(static_cast<unsigned char>(rhs[i]))) {
				return false;
			}
		}
		return true;
	}
};

struct move_only_key {
	std::unique_ptr<uint32> value;
	bool operator==(const move_only_key& other) const {
		return *value == *other.value;
	}
};

struct move_only_hasher {
	uint64 operator()(const move_only_key& key) const noexcept {
		return fmix64(*key.value);
	}
};

struct test_struct {
	test_struct(int32_t, uint64_t) {};
};

int32 run_sonnet_map_tests() {
	try {
		bool predicate_result{ [] {
			std::unordered_map<uint32, std::string> src;
			src[1] = "a";
			src[2] = "b";
			std::unordered_map<uint32, std::string> dst;
			dst = src;
			return dst.size() == 2 && dst[1] == "a" && dst[2] == "b";
		}() };

		unit_test<"sonnet: Copy assignment preserves size">::assert_eq(predicate_result, [] {
			sonnet_map<uint32, std::string> src{};
			src[1U] = "a";
			src[2U] = "b";
			sonnet_map<uint32, std::string> dst{};
			dst = src;
			return dst.size() == 2 && dst[1U] == "a" && dst[2U] == "b";
		});

		predicate_result = [] {
			std::unordered_map<uint32, std::string> map;
			for (uint32 i = 0; i < 64; ++i) {
				map.emplace(std::pair{ i, "val" });
			}
			bool all_found = true;
			for (uint32 i = 0; i < 64; ++i) {
				auto it = map.find(i);
				if (it == map.end() || it->second != "val") {
					all_found = false;
					break;
				}
			}
			bool non_existent_absent = (map.find(999999U) == map.end());
			return all_found && non_existent_absent && (map.size() == 64);
		}();

		unit_test<"sonnet: Trailing padding uninitialized control bytes match">::assert_eq(predicate_result, [] {
			sonnet_map<uint32, std::string> map{ 64 };
			for (uint32 i = 0; i < 64; ++i) {
				map.insert(pair{ i, "val" });
			}
			bool all_found = true;
			for (uint32 i = 0; i < 64; ++i) {
				auto it = map.find(i);
				if (it == map.end() || it->second != "val") {
					all_found = false;
					break;
				}
			}
			bool non_existent_absent = (map.find(999999U) == map.end());
			return all_found && non_existent_absent && (map.size() == 64);
		});

		predicate_result = [] {
			std::unordered_map<uint32, std::string> a;
			a[1] = "alpha";
			a[2] = "beta";

			std::unordered_map<uint32, std::string> b;
			b[10] = "x";
			b[20] = "y";
			b[30] = "z";

			b = std::move(a);

			return b.size() == 2 && b[1] == "alpha" && b[2] == "beta" && (b.find(10) == b.end());
		}();

		unit_test<"sonnet: Move-assignment lifecycle on pre-existing elements">::assert_eq(predicate_result, [] {
			sonnet_map<uint32, std::string> a;
			a[1U] = "alpha";
			a[2U] = "beta";

			sonnet_map<uint32, std::string> b;
			b[10U] = "x";
			b[20U] = "y";
			b[30U] = "z";

			b = std::move(a);

			return b.size() == 2 && b[1U] == "alpha" && b[2U] == "beta" && (b.find(10U) == b.end());
		});

		predicate_result = [] {
			std::unordered_map<uint32, uint32> map;
			for (uint32 i = 0; i < 500; ++i) {
				map.emplace(std::pair{ i, i * 2 });
			}
			for (uint32 i = 0; i < 400; ++i) {
				map.erase(i);
			}
			for (uint32 i = 500; i < 900; ++i) {
				map.emplace(std::pair{ i, i * 3 });
			}

			bool correct = true;
			for (uint32 i = 0; i < 400; ++i) {
				if (map.find(i) != map.end()) {
					correct = false;
					break;
				}
			}
			for (uint32 i = 400; i < 500; ++i) {
				auto it = map.find(i);
				if (it == map.end() || it->second != i * 2) {
					correct = false;
					break;
				}
			}
			for (uint32 i = 500; i < 900; ++i) {
				auto it = map.find(i);
				if (it == map.end() || it->second != i * 3) {
					correct = false;
					break;
				}
			}
			return correct && (map.size() == 500);
		}();

		unit_test<"sonnet: Heavy erase-insert churn tombstone fill and find">::assert_eq(predicate_result, [] {
			sonnet_map<uint32, uint32> map;
			for (uint32 i = 0; i < 500; ++i) {
				map.insert(pair{ i, i * 2 });
			}
			for (uint32 i = 0; i < 400; ++i) {
				map.erase(i);
			}
			for (uint32 i = 500; i < 900; ++i) {
				map.insert(pair{ i, i * 3 });
			}

			bool correct = true;
			for (uint32 i = 0; i < 400; ++i) {
				if (map.find(i) != map.end()) {
					correct = false;
					break;
				}
			}
			for (uint32 i = 400; i < 500; ++i) {
				auto it = map.find(i);
				if (it == map.end() || it->second != i * 2) {
					correct = false;
					break;
				}
			}
			for (uint32 i = 500; i < 900; ++i) {
				auto it = map.find(i);
				if (it == map.end() || it->second != i * 3) {
					correct = false;
					break;
				}
			}
			return correct && (map.size() == 500);
		});

		predicate_result = [] {
			std::unordered_map<uint32, uint32> map;
			for (uint32 i = 0; i < 16; ++i) {
				map[i * 1024U] = i;
			}
			bool matches = true;
			for (uint32 i = 0; i < 16; ++i) {
				if (map[i * 1024U] != i) {
					matches = false;
					break;
				}
			}
			return matches && (map.size() == 16);
		}();

		unit_test<"sonnet: Sub-bucket initial capacity power-of-two group wrap">::assert_eq(predicate_result, [] {
			sonnet_map<uint32, uint32> map{ 1 };
			for (uint32 i = 0; i < 16; ++i) {
				map[i * 1024U] = i;
			}
			bool matches = true;
			for (uint32 i = 0; i < 16; ++i) {
				if (map[i * 1024U] != i) {
					matches = false;
					break;
				}
			}
			return matches && (map.size() == 16);
		});

		predicate_result = [] {
			std::unordered_map<uint32, std::string> map;
			map.emplace(std::pair{ 42U, "hello" });
			map.emplace(std::pair{ 100U, "world" });
			return map[42] == "hello" && map[100] == "world" && map.size() == 2;
		}();

		unit_test<"sonnet: Basic insert and lookup">::assert_eq(predicate_result, []() {
			sonnet_map<uint32, std::string> map;
			map.insert(pair{ 42U, "hello" });
			map.insert(pair{ 100U, "world" });
			return map[42U] == "hello" && map[100U] == "world" && map.size() == 2;
		});

		predicate_result = [] {
			std::unordered_map<uint32, std::string> map;
			map[1]		 = "a";
			map[2]		 = "b";
			map[3]		 = "c";
			map[4]		 = "d";
			uint32 count = 0;
			for ([[maybe_unused]] auto& pair: map) {
				count++;
			}
			return count == 4;
		}();

		unit_test<"sonnet: Iterator traverses all elements">::assert_eq(predicate_result, [] {
			sonnet_map<uint32, std::string> map{};
			map[1U]		 = "a";
			map[2U]		 = "b";
			map[3U]		 = "c";
			map[4U]		 = "d";
			uint32 count = 0;
			for ([[maybe_unused]] auto& pair: map) {
				count++;
			}
			return count == 4;
		});

		predicate_result = [] {
			std::unordered_map<uint32, uint32> map;
			for (uint32 i = 0; i < 100; ++i) {
				map[i] = i * 2;
			}
			uint32 sum = 0;
			for (auto& [key, val]: map) {
				sum += key;
			}
			return sum == 4950;
		}();

		unit_test<"sonnet: Iterator finds all inserted keys">::assert_eq(predicate_result, [] {
			sonnet_map<uint32, uint32> map{};
			for (uint32 i = 0; i < 100; ++i) {
				map[i] = i * 2;
			}
			uint32 sum = 0;
			for (auto& [key, val]: map) {
				sum += key;
			}
			return sum == 4950;
		});

		predicate_result = [] {
			std::unordered_map<uint32, std::string> map;
			uint32 count = 0;
			for ([[maybe_unused]] auto& pair: map) {
				count++;
			}
			return count == 0;
		}();

		unit_test<"sonnet: Iterator on empty map">::assert_eq(predicate_result, [] {
			sonnet_map<uint32, std::string> map{};
			uint32 count = 0;
			for ([[maybe_unused]] auto& pair: map) {
				count++;
			}
			return count == 0;
		});

		predicate_result = [] {
			std::unordered_map<uint32, std::string> map;
			map[42]			 = "answer";
			uint32 count	 = 0;
			uint32 key_found = 0;
			for (auto& [key, val]: map) {
				count++;
				key_found = key;
			}
			return count == 1 && key_found == 42;
		}();

		unit_test<"sonnet: Iterator on single element">::assert_eq(predicate_result, [] {
			sonnet_map<uint32, std::string> map{};
			map[42U]		 = "answer";
			uint32 count	 = 0;
			uint32 key_found = 0;
			for (auto& [key, val]: map) {
				count++;
				key_found = key;
			}
			return count == 1 && key_found == 42;
		});

		predicate_result = [] {
			std::unordered_map<uint32, std::string> map;
			map[1] = "a";
			map[2] = "b";
			map[3] = "c";
			map[4] = "d";
			map[5] = "e";
			map.erase(2);
			map.erase(4);
			uint32 count = 0;
			for ([[maybe_unused]] auto& pair: map) {
				count++;
			}
			return count == 3;
		}();

		unit_test<"sonnet: Iterator after erase (tombstones)">::assert_eq(predicate_result, [] {
			sonnet_map<uint32, std::string> map{};
			map[1U] = "a";
			map[2U] = "b";
			map[3U] = "c";
			map[4U] = "d";
			map[5U] = "e";
			map.erase(2U);
			map.erase(4U);
			uint32 count = 0;
			for ([[maybe_unused]] auto& pair: map) {
				count++;
			}
			return count == 3;
		});

		predicate_result = [] {
			std::unordered_map<uint32, std::string> map;
			map[10]		 = "ten";
			map[20]		 = "twenty";
			map[30]		 = "thirty";
			auto it		 = map.begin();
			uint32 count = 0;
			while (it != map.end()) {
				count++;
				++it;
			}
			return count == 3;
		}();

		unit_test<"sonnet: Iterator manual increment">::assert_eq(predicate_result, [] {
			sonnet_map<uint32, std::string> map{};
			map[10U]	 = "ten";
			map[20U]	 = "twenty";
			map[30U]	 = "thirty";
			auto it		 = map.begin();
			uint32 count = 0;
			while (it != map.end()) {
				count++;
				++it;
			}
			return count == 3;
		});

		predicate_result = [] {
			std::unordered_map<uint32, uint32> map;
			map[0]		 = 0;
			map[1000]	 = 1000;
			map[2000]	 = 2000;
			uint32 count = 0;
			for ([[maybe_unused]] auto& pair: map) {
				count++;
			}
			return count == 3;
		}();

		unit_test<"sonnet: Iterator skips large gaps (sparse map)">::assert_eq(predicate_result, [] {
			sonnet_map<uint32, uint32> map{};
			map[0U]		 = 0U;
			map[1000U]	 = 1000U;
			map[2000U]	 = 2000U;
			uint32 count = 0;
			for ([[maybe_unused]] auto& pair: map) {
				count++;
			}
			return count == 3;
		});

		predicate_result = [] {
			std::unordered_map<uint32, std::string> map;
			map[42] = "first";
			map[42] = "updated";
			return map[42] == "updated" && map.size() == 1;
		}();

		unit_test<"sonnet: Update existing key">::assert_eq(predicate_result, []() {
			sonnet_map<uint32, std::string> map;
			map[42U] = "first";
			map[42U] = "updated";
			return map[42U] == "updated" && map.size() == 1;
		});

		predicate_result = [] {
			std::unordered_map<uint32, std::string> map;
			map[42] = "exists";
			return map.count(42) > 0 && map.count(999) == 0;
		}();

		unit_test<"sonnet: Contains check">::assert_eq(predicate_result, []() {
			sonnet_map<uint32, std::string> map;
			map[42U] = "exists";
			return map.contains(42) && !map.contains(999);
		});

		predicate_result = [] {
			std::unordered_map<uint32, std::string> map;
			map[42]	 = "value";
			auto val = map[999];
			return val.empty() && map.size() == 2;
		}();

		unit_test<"sonnet: operator[] auto-insert">::assert_eq(predicate_result, []() {
			sonnet_map<uint32, std::string> map;
			map[42U] = "value";
			auto val = map[999U];
			return val.empty() && map.size() == 2;
		});

		predicate_result = [] {
			std::unordered_map<uint32, uint32> map;
			for (uint32 i = 0; i < 10000; ++i) {
				map[i] = i * 2;
			}
			return map.size() == 10000 && map[5000] == 10000;
		}();

		unit_test<"sonnet: Large insertion stress test">::assert_eq(predicate_result, []() {
			sonnet_map<uint32, uint32> map;
			for (uint32 i = 0; i < 10000; ++i) {
				map[i] = i * 2;
			}
			return map.size() == 10000 && map[5000U] == 10000;
		});

		predicate_result = [] {
			std::unordered_map<uint32, std::string> map;
			map[1] = "one";
			map[2] = "two";
			map[3] = "three";
			map.erase(2);
			return map.size() == 2 && map.count(2) == 0 && map.count(1) > 0;
		}();

		unit_test<"sonnet: Tombstone erase functionality">::assert_eq(predicate_result, []() {
			sonnet_map<uint32, std::string> map;
			map[1U] = "one";
			map[2U] = "two";
			map[3U] = "three";
			map.erase(2);
			return map.size() == 2 && !map.contains(2) && map.contains(1);
		});

		predicate_result = [] {
			std::unordered_map<uint32, uint32> map;
			for (uint32 i = 0; i < 1000; ++i) {
				map[i] = i * 3;
			}
			return map[500] == 1500;
		}();

		unit_test<"sonnet: uint8 control bytes - max hash entropy">::assert_eq(predicate_result, []() {
			sonnet_map<uint32, uint32> map;
			for (uint32 i = 0; i < 1000; ++i) {
				map[i] = i * 3;
			}
			return map[500U] == 1500;
		});

		predicate_result = [] {
			std::unordered_map<uint32, std::string> map;
			for (uint32 i = 0; i < 100; i += 16) {
				map.insert(std::pair{ i, "value_" + std::string{ std::to_string(i).data() } });
			}
			return map[0] == "value_0" && map[64] == "value_64" && map.size() == 7;
		}();

		unit_test<"sonnet: Forced hash collisions - same bucket">::assert_eq(predicate_result, []() {
			sonnet_map<uint32, std::string> map(16ULL);
			for (uint32 i = 0; i < 100; i += 16) {
				map.insert(pair{ i, "value_" + std::string{ std::to_string(i).data() } });
			}
			return map[0U] == "value_0" && map[64U] == "value_64" && map.size() == 7;
		});

		predicate_result = [] {
			std::unordered_map<uint32, uint32> map;
			for (uint32 i = 0; i < 32; ++i) {
				map[i] = i * 10;
			}
			bool all_present = true;
			for (uint32 i = 0; i < 32; ++i) {
				if (map[i] != i * 10)
					all_present = false;
			}
			return all_present && map.size() == 32;
		}();

		unit_test<"sonnet: Dense packing - fill entire SIMD group">::assert_eq(predicate_result, []() {
			sonnet_map<uint32, uint32> map(64);
			for (uint32 i = 0; i < 32; ++i) {
				map[i] = i * 10;
			}
			bool all_present = true;
			for (uint32 i = 0; i < 32; ++i) {
				if (map[i] != i * 10)
					all_present = false;
			}
			return all_present && map.size() == 32;
		});

		predicate_result = [] {
			std::unordered_map<uint32, uint32> map;
			for (uint32 i = 0; i < 1000; i += 7) {
				map[i] = i;
			}
			return map[700] == 700 && map[707] == 707 && map.size() == 143;
		}();

		unit_test<"sonnet: Collision chain - linear probing verification">::assert_eq(predicate_result, []() {
			sonnet_map<uint32, uint32> map(128);
			for (uint32 i = 0; i < 1000; i += 7) {
				map[i] = i;
			}
			return map[700U] == 700 && map[707U] == 707 && map.size() == 143;
		});

		predicate_result = [] {
			std::unordered_map<uint32, std::string> map;
			for (uint32 i = 0; i < 50; ++i) {
				map[i] = std::string{ std::to_string(i).data() };
			}
			return map[0] == "0" && map[49] == "49";
		}();

		unit_test<"sonnet: Wraparound at table boundary">::assert_eq(predicate_result, []() {
			sonnet_map<uint32, std::string> map(64);
			for (uint32 i = 0; i < 50; ++i) {
				map[i] = std::string{ std::to_string(i).data() };
			}
			return map[0U] == "0" && map[49U] == "49";
		});

		predicate_result = [] {
			std::unordered_map<uint64_t, uint64_t> map;
			for (uint64_t i = 0; i < 100; ++i) {
				uint64_t key = (i << 57) | i;
				map[key]	 = i;
			}
			uint64_t test_key = (50ULL << 57) | 50;
			return map[test_key] == 50 && map.size() == 100;
		}();

		unit_test<"sonnet: Multiple h2 matches in same SIMD group">::assert_eq(predicate_result, []() {
			sonnet_map<uint64_t, uint64_t> map;
			for (uint64_t i = 0; i < 100; ++i) {
				uint64_t key = (i << 57) | i;
				map[key]	 = i;
			}
			uint64_t test_key = (50ULL << 57) | 50;
			return map[test_key] == 50 && map.size() == 100;
		});

		predicate_result = [] {
			std::unordered_map<uint32, uint32> map;
			for (uint32 i = 0; i < 50; ++i) {
				map[i] = i * 2;
			}
			for (uint32 i = 10; i < 20; ++i) {
				map.erase(i);
			}
			for (uint32 i = 10; i < 20; ++i) {
				map[i] = i * 3;
			}
			return map[15U] == 45 && map.size() == 50;
		}();

		unit_test<"sonnet: Collision + erase + reinsert with tombstones">::assert_eq(predicate_result, []() {
			sonnet_map<uint32, uint32> map;
			for (uint32 i = 0; i < 50; ++i) {
				map[i] = i * 2;
			}
			for (uint32 i = 10; i < 20; ++i) {
				map.erase(i);
			}
			for (uint32 i = 10; i < 20; ++i) {
				map[i] = i * 3;
			}
			return map[15U] == 45 && map.size() == 50;
		});

		predicate_result = [] {
			std::unordered_map<uint32, uint32> map;
			for (uint32 i = 0; i < 16; ++i) {
				uint32 key = 1 << i;
				map.insert({ key, i });
			}
			return map[1024] == 10 && map[32768] == 15;
		}();

		unit_test<"sonnet: Extreme collision - power of 2 keys">::assert_eq(predicate_result, []() {
			sonnet_map<uint32, uint32> map;
			for (uint32 i = 0; i < 16; ++i) {
				uint32 key = 1 << i;
				map.insert(pair{ key, i });
			}
			return map[1024U] == 10 && map[32768U] == 15;
		});
		predicate_result = [] {
			std::unordered_map<uint32, std::string> src;
			src[1] = "a";
			src[2] = "b";
			std::unordered_map<uint32, std::string> dst;
			dst = src;
			return dst.size() == 2 && dst[1] == "a" && dst[2] == "b";
		}();

		unit_test<"sonnet: Copy assignment preserves size">::assert_eq(predicate_result, [] {
			sonnet_map<uint32, std::string> src{};
			src[1U] = "a";
			src[2U] = "b";
			sonnet_map<uint32, std::string> dst{};
			dst = src;
			return dst.size() == 2 && dst[1U] == "a" && dst[2U] == "b";
		});

		predicate_result = [] {
			std::unordered_map<uint32, std::string> map;
			map.insert({ 42U, "hello" });
			map.insert({ 100U, "world" });
			return map[42] == "hello" && map[100] == "world" && map.size() == 2;
		}();

		unit_test<"sonnet: Basic insert and lookup">::assert_eq(predicate_result, []() {
			sonnet_map<uint32, std::string> map;
			map.insert(pair{ 42U, std::string{ "hello" } });
			map.insert(pair{ 100U, std::string{ "world" } });
			return map[42U] == "hello" && map[100U] == "world" && map.size() == 2;
		});

		predicate_result = [] {
			std::unordered_map<uint32, std::string> map;
			map[42] = "first";
			map[42] = "updated";
			return map[42] == "updated" && map.size() == 1;
		}();

		unit_test<"sonnet: Update existing key">::assert_eq(predicate_result, []() {
			sonnet_map<uint32, std::string> map;
			map[42U] = "first";
			map[42U] = "updated";
			return map[42U] == "updated" && map.size() == 1;
		});

		predicate_result = [] {
			std::unordered_map<uint32, std::string> map;
			map[42] = "exists";
			return map.count(42) > 0 && map.count(999) == 0;
		}();

		unit_test<"sonnet: Contains check">::assert_eq(predicate_result, []() {
			sonnet_map<uint32, std::string> map;
			map[42U] = "exists";
			return map.contains(42) && !map.contains(999);
		});

		predicate_result = [] {
			std::unordered_map<uint32, std::string> map;
			map[42]	 = "value";
			auto val = map[999];
			return val.empty() && map.size() == 2;
		}();

		unit_test<"sonnet: operator[] auto-insert">::assert_eq(predicate_result, []() {
			sonnet_map<uint32, std::string> map;
			map[42U] = "value";
			auto val = map[999U];
			return val.empty() && map.size() == 2;
		});

		predicate_result = [] {
			std::unordered_map<uint32, uint32> map;
			for (uint32 i = 0; i < 10000; ++i) {
				map[i] = i * 2;
			}
			return map.size() == 10000 && map[5000] == 10000;
		}();

		unit_test<"sonnet: Large insertion stress test">::assert_eq(predicate_result, []() {
			sonnet_map<uint32, uint32> map;
			for (uint32 i = 0; i < 10000; ++i) {
				map[i] = i * 2;
			}
			return map.size() == 10000 && map[5000U] == 10000;
		});

		predicate_result = [] {
			std::unordered_map<uint32, std::string> map;
			map[1] = "one";
			map[2] = "two";
			map[3] = "three";
			map.erase(2);
			return map.size() == 2 && map.count(2) == 0 && map.count(1) > 0;
		}();

		unit_test<"sonnet: Tombstone erase functionality">::assert_eq(predicate_result, []() {
			sonnet_map<uint32, std::string> map;
			map[1U] = "one";
			map[2U] = "two";
			map[3U] = "three";
			map.erase(2);
			return map.size() == 2 && !map.contains(2) && map.contains(1);
		});

		predicate_result = [] {
			std::unordered_map<uint32, uint32> map;
			for (uint32 i = 0; i < 1000; ++i) {
				map[i] = i * 3;
			}
			return map[500] == 1500;
		}();

		unit_test<"sonnet: uint8 control bytes - max hash entropy">::assert_eq(predicate_result, []() {
			sonnet_map<uint32, uint32> map;
			for (uint32 i = 0; i < 1000; ++i) {
				map[i] = i * 3;
			}
			return map[500U] == 1500;
		});

		predicate_result = [] {
			std::unordered_map<uint32, std::string> map;
			for (uint32 i = 0; i < 100; i += 16) {
				map.insert(std::pair{ i, "value_" + std::string{ std::to_string(i).data() } });
			}
			return map[0] == "value_0" && map[64] == "value_64" && map.size() == 7;
		}();

		unit_test<"sonnet: Forced hash collisions - same bucket">::assert_eq(predicate_result, []() {
			sonnet_map<uint32, std::string> map(16ULL);
			for (uint32 i = 0; i < 100; i += 16) {
				map.insert(pair{ i, "value_" + std::string{ std::to_string(i).data() } });
			}
			return map[0U] == "value_0" && map[64U] == "value_64" && map.size() == 7;
		});

		predicate_result = [] {
			std::unordered_map<uint32, uint32> map;
			for (uint32 i = 0; i < 32; ++i) {
				map[i] = i * 10;
			}
			bool all_present = true;
			for (uint32 i = 0; i < 32; ++i) {
				if (map[i] != i * 10)
					all_present = false;
			}
			return all_present && map.size() == 32;
		}();

		unit_test<"sonnet: Dense packing - fill entire SIMD group">::assert_eq(predicate_result, []() {
			sonnet_map<uint32, uint32> map(64);
			for (uint32 i = 0; i < 32; ++i) {
				map[i] = i * 10;
			}
			bool all_present = true;
			for (uint32 i = 0; i < 32; ++i) {
				if (map[i] != i * 10)
					all_present = false;
			}
			return all_present && map.size() == 32;
		});

		predicate_result = [] {
			std::unordered_map<uint32, uint32> map;
			for (uint32 i = 0; i < 1000; i += 7) {
				map[i] = i;
			}
			return map[700] == 700 && map[707] == 707 && map.size() == 143;
		}();

		unit_test<"sonnet: Collision chain - linear probing verification">::assert_eq(predicate_result, []() {
			sonnet_map<uint32, uint32> map(128);
			for (uint32 i = 0; i < 1000; i += 7) {
				map[i] = i;
			}
			return map[700U] == 700 && map[707U] == 707 && map.size() == 143;
		});

		predicate_result = [] {
			std::unordered_map<uint32, std::string> map;
			for (uint32 i = 0; i < 50; ++i) {
				map[i] = std::string{ std::to_string(i).data() };
			}
			return map[0] == "0" && map[49] == "49";
		}();

		unit_test<"sonnet: Wraparound at table boundary">::assert_eq(predicate_result, []() {
			sonnet_map<uint32, std::string> map(64);
			for (uint32 i = 0; i < 50; ++i) {
				map[i] = std::string{ std::to_string(i).data() };
			}
			return map[0U] == "0" && map[49U] == "49";
		});

		predicate_result = [] {
			std::unordered_map<uint64_t, uint64_t> map;
			for (uint64_t i = 0; i < 100; ++i) {
				uint64_t key = (i << 57) | i;
				map[key]	 = i;
			}
			uint64_t test_key = (50ULL << 57) | 50;
			return map[test_key] == 50 && map.size() == 100;
		}();

		unit_test<"sonnet: Multiple h2 matches in same SIMD group">::assert_eq(predicate_result, []() {
			sonnet_map<uint64_t, uint64_t> map;
			for (uint64_t i = 0; i < 100; ++i) {
				uint64_t key = (i << 57) | i;
				map[key]	 = i;
			}
			uint64_t test_key = (50ULL << 57) | 50;
			return map[test_key] == 50 && map.size() == 100;
		});

		predicate_result = [] {
			std::unordered_map<uint32, uint32> map;
			map[1U] = 10;
			map[2U] = 20;
			map.erase(1U);
			map[2U] = 30;
			return map.size() == 1 && map[2U] == 30;
		}();

		unit_test<"sonnet: Cross-group boundary tombstone duplicate exploit">::assert_eq(predicate_result, []() {
			sonnet_map<uint32, uint32> map;

			uint32 group_width = 16;
			for (uint32 i = 0; i < group_width + 5; ++i) {
				map[i] = i * 2;
			}

			map.erase(2U);

			uint32 overflow_key = group_width + 2;
			map[overflow_key]	= 999;

			return map.size() == (group_width + 4) && map[overflow_key] == 999;
		});

		predicate_result = [] {
			std::unordered_map<uint32, uint32> map;

			map[42U] = 100;
			map[99U] = 200;
			map.erase(42U);

			map[99U] = 300;

			return map.size() == 1 && map[99U] == 300;
		}();

		unit_test<"sonnet: Early termination tombstone duplicate exploit">::assert_eq(predicate_result, []() {
			sonnet_map<uint32, uint32> map;

			uint32 keyA = 5U;
			uint32 keyB = 5U + 32U;

			for (uint32 i = 0; i < 16; ++i) {
				map[i] = i;
			}

			map.erase(2U);

			map[14U] = 999;

			return map.size() == 15 && map[14U] == 999;
		});

		unit_test<"sonnet: Basic insert/find single value">::assert_eq(true, [] {
			sonnet_map<uint32, uint32> map;
			map[123] = 456;

			return map.contains(123) && map[123] == 456 && map.size() == 1;
		});

		unit_test<"sonnet: Initialization state verification">::assert_eq(true, [] {
			sonnet_map<uint32, uint32> map(16);

			return map.size() == 0 && map.capacity() >= 16;
		});

		unit_test<"sonnet: Multiple discrete insertions">::assert_eq(true, [] {
			sonnet_map<uint32, uint32> map;
			map[13] = 1337;
			map[42] = 2442;

			return map.size() == 2 && map[13] == 1337 && map[42] == 2442;
		});

		unit_test<"sonnet: In-place value update mechanics">::assert_eq(true, [] {
			sonnet_map<uint32, uint32> map;
			map[10] = 500;
			map[10] = 900;

			return map.size() == 1 && map[10] == 900;
		});

		unit_test<"sonnet: Alternative explicit insert internal pair syntax">::assert_eq(true, [] {
			sonnet_map<uint32, uint32> map;
			map.insert(55U, 888U);

			return map.size() == 1 && map.contains(55U) && map[55U] == 888U;
		});

		unit_test<"sonnet: Structural erase boundaries and tracking">::assert_eq(true, [] {
			sonnet_map<uint32, uint32> map;
			map[100]		  = 1;
			map[200]		  = 2;
			bool first_erase  = map.erase(100U);
			bool second_erase = map.erase(100U);

			return first_erase == true && second_erase == false && map.size() == 1 && !map.contains(100U);
		});

		unit_test<"sonnet: Load factor scaling and sequential rehash integrity">::assert_eq(true, [] {
			sonnet_map<uint32, uint32> map(4);
			uint64 initial_capacity = map.capacity();
			for (uint32 i = 0; i < 100; ++i) {
				map[i] = i * 2;
			}
			bool verified = (map.size() == 100) && (map.capacity() > initial_capacity);
			for (uint32 i = 0; i < 100; ++i) {
				if (map[i] != i * 2)
					verified = false;
			}

			return verified;
		});

		unit_test<"sonnet: Global clear tracking and iterator reset">::assert_eq(true, [] {
			sonnet_map<uint32, uint32> map;
			for (uint32 i = 0; i < 10; ++i) {
				map[i] = i;
			}
			map.clear();

			return map.size() == 0 && !map.contains(0U) && map.begin() == map.end();
		});

		unit_test<"sonnet: Layout linear tracking via forward iterators">::assert_eq(true, [] {
			sonnet_map<uint32, uint32> map;
			map[1]				 = 10;
			map[2]				 = 20;
			map[3]				 = 30;
			uint32 sum_keys		 = 0;
			uint32 sum_vals		 = 0;
			uint32 element_count = 0;
			for (auto it = map.begin(); it != map.end(); ++it) {
				sum_keys += it->first;
				sum_vals += it->second;
				element_count++;
			}

			return element_count == 3 && sum_keys == 6 && sum_vals == 60;
		});

		unit_test<"sonnet: Active iterator lookup translation and direct removal">::assert_eq(true, [] {
			sonnet_map<uint32, uint32> map;
			map[50]		= 5000;
			auto it		= map.find(50U);
			bool erased = map.erase(it);

			return erased && map.size() == 0 && !map.contains(50U);
		});

		unit_test<"sonnet: Deep copy assignment data isolation">::assert_eq(true, [] {
			sonnet_map<uint32, uint32> map1;
			map1[1] = 100;
			map1[2] = 200;
			sonnet_map<uint32, uint32> map2;
			map2 = map1;

			return map2.size() == 2 && map2[1] == 100 && map2[2] == 200 && map1.size() == 2;
		});

		unit_test<"sonnet: Move constructor storage allocation transfer">::assert_eq(true, [] {
			sonnet_map<uint32, uint32> map1;
			map1[5] = 55;
			sonnet_map<uint32, uint32> map2(std::move(map1));

			return map2.size() == 1 && map2[5] == 55 && map1.size() == 0 && map1.capacity() == 0;
		});

		unit_test<"sonnet: Move assignment resources clearance pipeline">::assert_eq(true, [] {
			sonnet_map<uint32, uint32> map1;
			map1[8] = 88;
			sonnet_map<uint32, uint32> map2;
			map2 = std::move(map1);

			return map2.size() == 1 && map2[8] == 88 && map1.size() == 0 && map1.capacity() == 0;
		});

		unit_test<"sonnet: Dynamic reporting profile for baseline storage footprints">::assert_eq(true, [] {
			sonnet_map<uint32, uint32> map(32);
			uint64 base_empty_memory = map.total_memory_usage();
			map[1]					 = 10;
			uint64 populated_memory	 = map.total_memory_usage();

			return base_empty_memory > 0 && populated_memory >= base_empty_memory;
		});

		unit_test<"sonnet: Wrap-around multi-group simd probe layout stability">::assert_eq(true, [] {
			sonnet_map<uint32, uint32> map(16);
			uint64 capacity = map.capacity();
			for (uint32 i = 0; i < capacity / 2; ++i) {
				map[i] = i * 10;
			}
			bool integrity = true;
			for (uint32 i = 0; i < capacity / 2; ++i) {
				if (!map.contains(i) || map[i] != i * 10) {
					integrity = false;
				}
			}

			return integrity;
		});

		unit_test<"sonnet: Overwrite sequence recycling and tombstone re-use efficiency">::assert_eq(true, [] {
			sonnet_map<uint32, uint32> map(16);
			for (uint32 i = 0; i < 8; ++i) {
				map[i] = i;
			}
			for (uint32 i = 0; i < 4; ++i) {
				map.erase(i);
			}
			for (uint32 i = 0; i < 4; ++i) {
				map[i + 20] = i + 20;
			}
			bool valid = (map.size() == 8);
			for (uint32 i = 4; i < 8; ++i) {
				if (!map.contains(i))
					valid = false;
			}
			for (uint32 i = 0; i < 4; ++i) {
				if (!map.contains(i + 20))
					valid = false;
			}

			return valid;
		});

		unit_test<"sonnet: cross-group tombstone reuse must not duplicate an existing key">::assert_eq(true, [] {
			sonnet_map<uint32, uint32> map(4096);

			using hasher_t = hasher<uint32, rt_key_hasher<uint32>>;
			hasher_t h{};
			const uint64 num_groups = map.capacity() / (cpu_properties::get_value(cpu_property_types::alignment));
			auto home				= [&](uint32 k) {
				return h.h1(h(k)) & (num_groups - 1);
			};

			uint32 a = 0, b = 0;
			for (uint32 i = 1; i < 100000 && !b; ++i)
				for (uint32 j = i + 1; j < i + 4000 && j < 100000; ++j)
					if (home(i) == home(j)) {
						a = i;
						b = j;
						break;
					}

			const uint64 width = cpu_properties::get_value(cpu_property_types::alignment);
			map[a]			   = a;
			uint32 pad = 200000, placed = 1;
			while (placed < width) {
				if (home(pad) == home(a)) {
					map[pad] = pad;
					++placed;
				}
				++pad;
			}
			map[b] = b;

			map.erase(a);
			const uint64 size_before = map.size();
			map[b]					 = 999;

			bool valid = (map.size() == size_before) && map.contains(b);
			return valid;
		});

		predicate_result = [] {
			std::unordered_map<uint32, uint32> map;
			for (uint32 i = 0; i < 50; ++i) {
				map[i] = i * 2;
			}
			for (uint32 i = 10; i < 20; ++i) {
				map.erase(i);
			}
			for (uint32 i = 10; i < 20; ++i) {
				map[i] = i * 3;
			}
			return map[15U] == 45 && map.size() == 50;
		}();

		unit_test<"sonnet: Collision + erase + reinsert with tombstones">::assert_eq(predicate_result, []() {
			sonnet_map<uint32, uint32> map;
			for (uint32 i = 0; i < 50; ++i) {
				map[i] = i * 2;
			}
			for (uint32 i = 10; i < 20; ++i) {
				map.erase(i);
			}
			for (uint32 i = 10; i < 20; ++i) {
				map[i] = i * 3;
			}
			return map[15U] == 45 && map.size() == 50;
		});

		unit_test<"sonnet: RAII lifetime tracking on erase, rehash, clear, and destruction">::assert_eq(true, [] {
			Tracked::alive = 0;
			{
				sonnet_map<uint32, Tracked> map(4);
				for (uint32 i = 0; i < 50; ++i) {
					map[i] = Tracked{ i * 10 };
				}
				if (Tracked::alive != 50)
					return false;

				for (uint32 i = 0; i < 20; ++i) {
					map.erase(i);
				}
				if (Tracked::alive != 30 || map.size() != 30)
					return false;

				for (uint32 i = 20; i < 50; ++i) {
					map[i] = Tracked{ i * 100 };
				}
				if (Tracked::alive != 30 || map.size() != 30)
					return false;

				for (uint32 i = 50; i < 100; ++i) {
					map[i] = Tracked{ i };
				}
				if (Tracked::alive != 80 || map.size() != 80)
					return false;

				map.clear();
				if (Tracked::alive != 0 || map.size() != 0)
					return false;
			}
			return Tracked::alive == 0;
		});

		unit_test<"sonnet: Move-only insert & lookup">::assert_eq(true, [] {
			sonnet_map<uint32, std::unique_ptr<uint32>> map;
			map.insert(pair{ 1U, std::make_unique<uint32>(100U) });
			map.insert(pair{ 2U, std::make_unique<uint32>(200U) });
			return map.size() == 2 && *map[1U] == 100U && *map[2U] == 200U;
		});

		unit_test<"sonnet: Move-only erase">::assert_eq(true, [] {
			sonnet_map<uint32, std::unique_ptr<uint32>> map;
			map.insert(pair{ 1U, std::make_unique<uint32>(100U) });
			map.insert(pair{ 2U, std::make_unique<uint32>(200U) });
			map.erase(1U);
			return map.size() == 1 && !map.contains(1U) && *map[2U] == 200U;
		});

		unit_test<"sonnet: Move-only move construction">::assert_eq(true, [] {
			sonnet_map<uint32, std::unique_ptr<uint32>> map;
			map.insert(pair{ 2U, std::make_unique<uint32>(200U) });
			sonnet_map<uint32, std::unique_ptr<uint32>> moved_map = std::move(map);
			return moved_map.size() == 1 && *moved_map[2U] == 200U && map.size() == 0;
		});

		unit_test<"sonnet: Move-only value semantics (std::unique_ptr)">::assert_eq(true, [] {
			sonnet_map<uint32, std::unique_ptr<uint32>> map;
			map.insert(pair{ 1U, std::make_unique<uint32>(100U) });
			map.insert(pair{ 2U, std::make_unique<uint32>(200U) });

			if (map.size() != 2 || *map[1U] != 100U || *map[2U] != 200U) {
				return false;
			}

			map.erase(1U);
			if (map.size() != 1 || map.contains(1U) || *map[2U] != 200U) {
				return false;
			}

			map[2U] = std::make_unique<uint32>(300U);
			if (map.size() != 1 || *map[2U] != 300U) {
				return false;
			}

			sonnet_map<uint32, std::unique_ptr<uint32>> moved_map = std::move(map);
			return moved_map.size() == 1 && *moved_map[2U] == 300U && map.size() == 0;
		});

		unit_test<"sonnet: Deep multi-group overflow tombstone duplicate check">::assert_eq(true, [] {
			sonnet_map<uint32, uint32> map(1024);

			using hasher_t = hasher<uint32, rt_key_hasher<uint32>>;
			hasher_t h{};
			const uint64 num_groups = map.capacity() / (cpu_properties::get_value(cpu_property_types::alignment));
			auto home				= [&](uint32 k) {
				return h.h1(h(k)) & (num_groups - 1);
			};

			uint32 target_group = 0;
			std::vector<uint32> colliding_keys;
			for (uint32 k = 1; colliding_keys.size() < 40 && k < 500000; ++k) {
				if (home(k) == target_group) {
					colliding_keys.push_back(k);
				}
			}

			if (colliding_keys.size() < 40)
				return false;

			for (size_t i = 0; i < colliding_keys.size(); ++i) {
				map[colliding_keys[i]] = static_cast<uint32>(i * 10);
			}

			uint32 early_key = colliding_keys[1];
			uint32 late_key	 = colliding_keys[35];

			map.erase(early_key);
			const uint64 size_after_erase = map.size();

			map[late_key] = 99999U;

			if (map.size() != size_after_erase)
				return false;
			if (map[late_key] != 99999U)
				return false;
			if (map.contains(early_key))
				return false;

			uint32 count = 0;
			for (auto& [k, v]: map) {
				if (k == late_key) {
					++count;
				}
			}

			return count == 1;
		});

		unit_test<"sonnet: Copy assignment from empty source clears destination">::assert_eq(true, [] {
			sonnet_map<uint32, uint32> dst;
			dst[1] = 10;
			dst[2] = 20;
			sonnet_map<uint32, uint32> src;
			dst = src;
			return dst.size() == 0 && !dst.contains(1U) && !dst.contains(2U);
		});

		predicate_result = [] {
			std::unordered_map<uint32, uint32> map;
			for (uint32 i = 0; i < 16; ++i) {
				uint32 key = 1 << i;
				map.insert({ key, i });
			}
			return map[1024] == 10 && map[32768] == 15;
		}();

		unit_test<"sonnet: Extreme collision - power of 2 keys">::assert_eq(predicate_result, []() {
			sonnet_map<uint32, uint32> map;
			for (uint32 i = 0; i < 16; ++i) {
				uint32 key = 1 << i;
				map.insert(pair{ key, i });
			}
			return map[1024U] == 10 && map[32768U] == 15;
		});
	} catch (const rt_ut::rt_ut_exception&) {
		throw;
	}

	uint64 failure_count{};
	auto tally = [&](bool result) {
		if (!result) {
			++failure_count;
		}
	};

	tally(unit_test<"sonnet: prefetch leaves lookups and size unchanged", true, false>::assert_eq(true, [] {
		sonnet_map<uint32, uint32> map{};
		for (uint32 i = 0; i < 100; ++i) {
			map[i] = i * 2;
		}
		bool intact = true;
		for (uint32 i = 0; i < 200; ++i) {
			map.prefetch(i);
			intact = intact && (map.contains(i) == (i < 100));
		}
		return intact && map.size() == 100 && map[7U] == 14;
	}));

	tally(unit_test<"sonnet: prefetch on an empty map is harmless", true, false>::assert_eq(true, [] {
		sonnet_map<uint32, uint32> map{};
		map.prefetch(1U);
		return map.size() == 0 && !map.contains(1U);
	}));

	tally(unit_test<"sonnet: default construction yields empty map", true, false>::assert_eq(true, [] {
		sonnet_map<uint32, uint32> map{};
		return map.size() == 0;
	}));

	tally(unit_test<"sonnet: default construction yields begin equal to end", true, false>::assert_eq(true, [] {
		sonnet_map<uint32, uint32> map{};
		return map.begin() == map.end();
	}));

	tally(unit_test<"sonnet: default construction does not allocate", true, false>::assert_eq(true, [] {
		sonnet_map<uint32, uint32> map{};
		return map.capacity() == 0 && map.element_ptr == nullptr;
	}));

	tally(unit_test<"sonnet: capacity is a power of two", true, false>::assert_eq(true, [] {
		sonnet_map<uint32, uint32> map{};
		const uint64 capacity = map.capacity();
		return (capacity & (capacity - 1)) == 0;
	}));

	tally(unit_test<"sonnet: construction hint produces capacity at least the hint", true, false>::assert_eq(true, [] {
		sonnet_map<uint32, uint32> map{ 500 };
		return map.capacity() >= 500;
	}));

	tally(unit_test<"sonnet: empty map contains reports false", true, false>::assert_eq(true, [] {
		sonnet_map<uint32, uint32> map{};
		return !map.contains(1U);
	}));

	tally(unit_test<"sonnet: empty map find returns end", true, false>::assert_eq(true, [] {
		sonnet_map<uint32, uint32> map{};
		return map.find(1U) == map.end();
	}));

	tally(unit_test<"sonnet: single insert increases size to one", true, false>::assert_eq(true, [] {
		sonnet_map<uint32, uint32> map{};
		map.insert(pair<uint32, uint32>{ 1U, 10U });
		return map.size() == 1;
	}));

	tally(unit_test<"sonnet: two argument insert stores the value", true, false>::assert_eq(true, [] {
		sonnet_map<uint32, uint32> map{};
		map.insert(1U, 10U);
		return map.size() == 1 && map[1U] == 10U;
	}));

	tally(unit_test<"sonnet: duplicate insert leaves size unchanged", true, false>::assert_eq(true, [] {
		sonnet_map<uint32, uint32> map{};
		map.insert(pair<uint32, uint32>{ 1U, 10U });
		map.insert(pair<uint32, uint32>{ 1U, 999U });
		return map.size() == 1;
	}));

	tally(unit_test<"sonnet: duplicate insert preserves the original value", true, false>::assert_eq(true, [] {
		sonnet_map<uint32, uint32> map{};
		map.insert(pair<uint32, uint32>{ 1U, 10U });
		map.insert(pair<uint32, uint32>{ 1U, 999U });
		return map[1U] == 10U;
	}));

	tally(unit_test<"sonnet: duplicate insert matches std::unordered_map semantics", true, false>::assert_eq(true, [] {
		std::unordered_map<uint32, uint32> oracle{};
		oracle.insert({ 1U, 10U });
		oracle.insert({ 1U, 999U });
		sonnet_map<uint32, uint32> map{};
		map.insert(pair<uint32, uint32>{ 1U, 10U });
		map.insert(pair<uint32, uint32>{ 1U, 999U });
		return map.size() == oracle.size() && map[1U] == oracle[1U];
	}));

	tally(unit_test<"sonnet: thousand sequential inserts retain every key", true, false>::assert_eq(true, [] {
		sonnet_map<uint32, uint32> map{};
		for (uint32 i = 0; i < 1000; ++i) {
			map.insert(pair<uint32, uint32>{ i, i * 3U });
		}
		if (map.size() != 1000) {
			return false;
		}
		for (uint32 i = 0; i < 1000; ++i) {
			if (!map.contains(i) || map[i] != i * 3U) {
				return false;
			}
		}
		return true;
	}));

	tally(unit_test<"sonnet: growth keeps capacity above size", true, false>::assert_eq(true, [] {
		sonnet_map<uint32, uint32> map{};
		for (uint32 i = 0; i < 1000; ++i) {
			map.insert(pair<uint32, uint32>{ i, i });
			if (map.capacity() < map.size()) {
				return false;
			}
		}
		return true;
	}));

	tally(unit_test<"sonnet: capacity never shrinks during growth", true, false>::assert_eq(true, [] {
		sonnet_map<uint32, uint32> map{};
		uint64 previous = map.capacity();
		for (uint32 i = 0; i < 500; ++i) {
			map.insert(pair<uint32, uint32>{ i, i });
			if (map.capacity() < previous) {
				return false;
			}
			previous = map.capacity();
		}
		return true;
	}));

	tally(unit_test<"sonnet: find returns the stored value", true, false>::assert_eq(true, [] {
		sonnet_map<uint32, uint32> map{};
		map.insert(pair<uint32, uint32>{ 7U, 77U });
		auto it = map.find(7U);
		return it != map.end() && (*it).second == 77U;
	}));

	tally(unit_test<"sonnet: find on a missing key returns end", true, false>::assert_eq(true, [] {
		sonnet_map<uint32, uint32> map{};
		map.insert(pair<uint32, uint32>{ 7U, 77U });
		return map.find(8U) == map.end();
	}));

	tally(unit_test<"sonnet: const find locates an existing key", true, false>::assert_eq(true, [] {
		sonnet_map<uint32, uint32> map{};
		map.insert(pair<uint32, uint32>{ 7U, 77U });
		const auto& const_map = map;
		auto it				  = const_map.find(7U);
		return it != const_map.end() && (*it).second == 77U;
	}));

	tally(unit_test<"sonnet: const find on a missing key returns end", true, false>::assert_eq(true, [] {
		sonnet_map<uint32, uint32> map{};
		map.insert(pair<uint32, uint32>{ 7U, 77U });
		const auto& const_map = map;
		return const_map.find(8U) == const_map.end();
	}));

	tally(unit_test<"sonnet: contains distinguishes present and absent keys", true, false>::assert_eq(true, [] {
		sonnet_map<uint32, uint32> map{};
		map.insert(pair<uint32, uint32>{ 7U, 77U });
		return map.contains(7U) && !map.contains(8U);
	}));

	tally(unit_test<"sonnet: subscript inserts a default value for a missing key", true, false>::assert_eq(true, [] {
		sonnet_map<uint32, uint32> map{};
		const uint32 value = map[42U];
		return value == 0 && map.size() == 1 && map.contains(42U);
	}));

	tally(unit_test<"sonnet: subscript returns an existing value without inserting", true, false>::assert_eq(true, [] {
		sonnet_map<uint32, uint32> map{};
		map.insert(pair<uint32, uint32>{ 42U, 5U });
		const uint32 value = map[42U];
		return value == 5U && map.size() == 1;
	}));

	tally(unit_test<"sonnet: mutation through a subscript reference persists", true, false>::assert_eq(true, [] {
		sonnet_map<uint32, uint32> map{};
		map[42U] = 7U;
		map[42U] = 9U;
		return map.size() == 1 && map[42U] == 9U;
	}));

	tally(unit_test<"sonnet: subscript matches std::unordered_map on a missing key", true, false>::assert_eq(true, [] {
		std::unordered_map<uint32, uint32> oracle{};
		sonnet_map<uint32, uint32> map{};
		const uint32 a = oracle[13U];
		const uint32 b = map[13U];
		return a == b && oracle.size() == map.size();
	}));

	tally(unit_test<"sonnet: erase of a present key returns true", true, false>::assert_eq(true, [] {
		sonnet_map<uint32, uint32> map{};
		map.insert(pair<uint32, uint32>{ 1U, 1U });
		return map.erase(1U);
	}));

	tally(unit_test<"sonnet: erase of an absent key returns false", true, false>::assert_eq(true, [] {
		sonnet_map<uint32, uint32> map{};
		map.insert(pair<uint32, uint32>{ 1U, 1U });
		return !map.erase(99U);
	}));

	tally(unit_test<"sonnet: erase decrements size", true, false>::assert_eq(true, [] {
		sonnet_map<uint32, uint32> map{};
		for (uint32 i = 0; i < 5; ++i) {
			map.insert(pair<uint32, uint32>{ i, i });
		}
		map.erase(2U);
		return map.size() == 4;
	}));

	tally(unit_test<"sonnet: erase of an absent key leaves size unchanged", true, false>::assert_eq(true, [] {
		sonnet_map<uint32, uint32> map{};
		for (uint32 i = 0; i < 5; ++i) {
			map.insert(pair<uint32, uint32>{ i, i });
		}
		map.erase(99U);
		return map.size() == 5;
	}));

	tally(unit_test<"sonnet: erased key is no longer contained", true, false>::assert_eq(true, [] {
		sonnet_map<uint32, uint32> map{};
		map.insert(pair<uint32, uint32>{ 1U, 1U });
		map.erase(1U);
		return !map.contains(1U) && map.find(1U) == map.end();
	}));

	tally(unit_test<"sonnet: second erase of the same key returns false", true, false>::assert_eq(true, [] {
		sonnet_map<uint32, uint32> map{};
		map.insert(pair<uint32, uint32>{ 1U, 1U });
		return map.erase(1U) && !map.erase(1U);
	}));

	tally(unit_test<"sonnet: erase through an iterator removes the entry", true, false>::assert_eq(true, [] {
		sonnet_map<uint32, uint32> map{};
		map.insert(pair<uint32, uint32>{ 1U, 1U });
		map.insert(pair<uint32, uint32>{ 2U, 2U });
		auto it = map.find(1U);
		map.erase(it);
		return map.size() == 1 && !map.contains(1U) && map.contains(2U);
	}));

	tally(unit_test<"sonnet: erasing every key empties the map", true, false>::assert_eq(true, [] {
		sonnet_map<uint32, uint32> map{};
		for (uint32 i = 0; i < 64; ++i) {
			map.insert(pair<uint32, uint32>{ i, i });
		}
		for (uint32 i = 0; i < 64; ++i) {
			map.erase(i);
		}
		return map.size() == 0 && map.begin() == map.end();
	}));

	tally(unit_test<"sonnet: erase then reinsert restores the key with a new value", true, false>::assert_eq(true, [] {
		sonnet_map<uint32, uint32> map{};
		map.insert(pair<uint32, uint32>{ 5U, 50U });
		map.erase(5U);
		map.insert(pair<uint32, uint32>{ 5U, 500U });
		return map.size() == 1 && map[5U] == 500U;
	}));

	tally(unit_test<"sonnet: repeated erase and reinsert keeps size stable", true, false>::assert_eq(true, [] {
		sonnet_map<uint32, uint32> map{};
		for (uint32 i = 0; i < 32; ++i) {
			map.insert(pair<uint32, uint32>{ i, i });
		}
		for (uint32 round = 0; round < 20; ++round) {
			for (uint32 i = 0; i < 16; ++i) {
				map.erase(i);
			}
			for (uint32 i = 0; i < 16; ++i) {
				map.insert(pair<uint32, uint32>{ i, i });
			}
			if (map.size() != 32) {
				return false;
			}
		}
		return true;
	}));

	tally(unit_test<"sonnet: iteration visits exactly size elements", true, false>::assert_eq(true, [] {
		sonnet_map<uint32, uint32> map{};
		for (uint32 i = 0; i < 100; ++i) {
			map.insert(pair<uint32, uint32>{ i, i });
		}
		uint64 seen = 0;
		for (auto& entry: map) {
			seen += entry.first * 0;
			++seen;
		}
		return seen == map.size();
	}));

	tally(unit_test<"sonnet: iteration visits each key exactly once", true, false>::assert_eq(true, [] {
		sonnet_map<uint32, uint32> map{};
		for (uint32 i = 0; i < 100; ++i) {
			map.insert(pair<uint32, uint32>{ i, i });
		}
		std::unordered_map<uint32, uint32> counts{};
		for (auto& entry: map) {
			++counts[entry.first];
		}
		if (counts.size() != 100) {
			return false;
		}
		for (auto& entry: counts) {
			if (entry.second != 1) {
				return false;
			}
		}
		return true;
	}));

	tally(unit_test<"sonnet: iteration yields the stored values", true, false>::assert_eq(true, [] {
		sonnet_map<uint32, uint32> map{};
		for (uint32 i = 0; i < 50; ++i) {
			map.insert(pair<uint32, uint32>{ i, i * 7U });
		}
		for (auto& [key, value]: map) {
			if (value != key * 7U) {
				return false;
			}
		}
		return true;
	}));

	tally(unit_test<"sonnet: iteration after partial erase skips tombstones", true, false>::assert_eq(true, [] {
		sonnet_map<uint32, uint32> map{};
		for (uint32 i = 0; i < 100; ++i) {
			map.insert(pair<uint32, uint32>{ i, i });
		}
		for (uint32 i = 0; i < 100; i += 2) {
			map.erase(i);
		}
		uint64 seen = 0;
		for (auto& [key, value]: map) {
			if ((key % 2) == 0 || value != key) {
				return false;
			}
			++seen;
		}
		return seen == 50 && map.size() == 50;
	}));

	tally(unit_test<"sonnet: const iteration visits exactly size elements", true, false>::assert_eq(true, [] {
		sonnet_map<uint32, uint32> map{};
		for (uint32 i = 0; i < 40; ++i) {
			map.insert(pair<uint32, uint32>{ i, i });
		}
		const auto& const_map = map;
		uint64 seen			  = 0;
		for (auto& entry: const_map) {
			seen += entry.first * 0;
			++seen;
		}
		return seen == 40;
	}));

	tally(unit_test<"sonnet: iterator post increment advances by one", true, false>::assert_eq(true, [] {
		sonnet_map<uint32, uint32> map{};
		for (uint32 i = 0; i < 4; ++i) {
			map.insert(pair<uint32, uint32>{ i, i });
		}
		auto it		  = map.begin();
		auto snapshot = it++;
		return snapshot != it;
	}));

	tally(unit_test<"sonnet: clear resets size to zero", true, false>::assert_eq(true, [] {
		sonnet_map<uint32, uint32> map{};
		for (uint32 i = 0; i < 20; ++i) {
			map.insert(pair<uint32, uint32>{ i, i });
		}
		map.clear();
		return map.size() == 0;
	}));

	tally(unit_test<"sonnet: clear removes every key", true, false>::assert_eq(true, [] {
		sonnet_map<uint32, uint32> map{};
		for (uint32 i = 0; i < 20; ++i) {
			map.insert(pair<uint32, uint32>{ i, i });
		}
		map.clear();
		for (uint32 i = 0; i < 20; ++i) {
			if (map.contains(i)) {
				return false;
			}
		}
		return map.begin() == map.end();
	}));

	tally(unit_test<"sonnet: insert after clear works", true, false>::assert_eq(true, [] {
		sonnet_map<uint32, uint32> map{};
		for (uint32 i = 0; i < 20; ++i) {
			map.insert(pair<uint32, uint32>{ i, i });
		}
		map.clear();
		map.insert(pair<uint32, uint32>{ 3U, 33U });
		return map.size() == 1 && map[3U] == 33U;
	}));

	tally(unit_test<"sonnet: clear preserves capacity", true, false>::assert_eq(true, [] {
		sonnet_map<uint32, uint32> map{};
		for (uint32 i = 0; i < 200; ++i) {
			map.insert(pair<uint32, uint32>{ i, i });
		}
		const uint64 capacity_before = map.capacity();
		map.clear();
		return map.capacity() == capacity_before;
	}));

	tally(unit_test<"sonnet: clear destroys every stored value exactly once", true, false>::assert_eq(true, [] {
		Tracked::alive = 0;
		sonnet_map<uint32, Tracked> map{};
		for (uint32 i = 0; i < 40; ++i) {
			map.insert(pair<uint32, Tracked>{ i, Tracked{ i } });
		}
		map.clear();
		if (Tracked::alive != 0) {
			return false;
		}
		return Tracked::alive == 0;
	}));

	tally(unit_test<"sonnet: destruction releases every stored value", true, false>::assert_eq(true, [] {
		Tracked::alive = 0;
		{
			sonnet_map<uint32, Tracked> map{};
			for (uint32 i = 0; i < 100; ++i) {
				map.insert(pair<uint32, Tracked>{ i, Tracked{ i } });
			}
		}
		return Tracked::alive == 0;
	}));

	tally(unit_test<"sonnet: erase destroys the removed value", true, false>::assert_eq(true, [] {
		Tracked::alive = 0;
		{
			sonnet_map<uint32, Tracked> map{};
			for (uint32 i = 0; i < 10; ++i) {
				map.insert(pair<uint32, Tracked>{ i, Tracked{ i } });
			}
			const uint32 before = Tracked::alive;
			map.erase(5U);
			if (Tracked::alive != before - 1) {
				return false;
			}
		}
		return Tracked::alive == 0;
	}));

	tally(unit_test<"sonnet: growth does not leak tracked values", true, false>::assert_eq(true, [] {
		Tracked::alive = 0;
		{
			sonnet_map<uint32, Tracked> map{};
			for (uint32 i = 0; i < 500; ++i) {
				map.insert(pair<uint32, Tracked>{ i, Tracked{ i } });
			}
			if (Tracked::alive != 500) {
				return false;
			}
		}
		return Tracked::alive == 0;
	}));

	tally(unit_test<"sonnet: copy construction produces an equal map", true, false>::assert_eq(true, [] {
		sonnet_map<uint32, uint32> source{};
		for (uint32 i = 0; i < 30; ++i) {
			source.insert(pair<uint32, uint32>{ i, i * 2U });
		}
		sonnet_map<uint32, uint32> copy{ source };
		if (copy.size() != source.size()) {
			return false;
		}
		for (uint32 i = 0; i < 30; ++i) {
			if (!copy.contains(i) || copy[i] != i * 2U) {
				return false;
			}
		}
		return true;
	}));

	tally(unit_test<"sonnet: copy construction produces an independent map", true, false>::assert_eq(true, [] {
		sonnet_map<uint32, uint32> source{};
		for (uint32 i = 0; i < 30; ++i) {
			source.insert(pair<uint32, uint32>{ i, i });
		}
		sonnet_map<uint32, uint32> copy{ source };
		copy.erase(0U);
		copy[1U] = 999U;
		return source.size() == 30 && source.contains(0U) && source[1U] == 1U;
	}));

	tally(unit_test<"sonnet: copy assignment produces an equal map", true, false>::assert_eq(true, [] {
		sonnet_map<uint32, uint32> source{};
		for (uint32 i = 0; i < 30; ++i) {
			source.insert(pair<uint32, uint32>{ i, i * 2U });
		}
		sonnet_map<uint32, uint32> destination{};
		destination = source;
		if (destination.size() != 30) {
			return false;
		}
		for (uint32 i = 0; i < 30; ++i) {
			if (destination[i] != i * 2U) {
				return false;
			}
		}
		return true;
	}));

	tally(unit_test<"sonnet: copy assignment discards prior destination contents", true, false>::assert_eq(true, [] {
		sonnet_map<uint32, uint32> source{};
		source.insert(pair<uint32, uint32>{ 1U, 1U });
		sonnet_map<uint32, uint32> destination{};
		for (uint32 i = 100; i < 120; ++i) {
			destination.insert(pair<uint32, uint32>{ i, i });
		}
		destination = source;
		return destination.size() == 1 && destination.contains(1U) && !destination.contains(100U);
	}));

	tally(unit_test<"sonnet: copy assignment is independent of the source", true, false>::assert_eq(true, [] {
		sonnet_map<uint32, uint32> source{};
		for (uint32 i = 0; i < 10; ++i) {
			source.insert(pair<uint32, uint32>{ i, i });
		}
		sonnet_map<uint32, uint32> destination{};
		destination = source;
		source.clear();
		return destination.size() == 10 && destination.contains(5U);
	}));

	tally(unit_test<"sonnet: copying an empty map yields an empty map", true, false>::assert_eq(true, [] {
		sonnet_map<uint32, uint32> source{};
		sonnet_map<uint32, uint32> copy{ source };
		return copy.size() == 0 && copy.begin() == copy.end();
	}));

	tally(unit_test<"sonnet: copy preserves tracked value count", true, false>::assert_eq(true, [] {
		Tracked::alive = 0;
		{
			sonnet_map<uint32, Tracked> source{};
			for (uint32 i = 0; i < 20; ++i) {
				source.insert(pair<uint32, Tracked>{ i, Tracked{ i } });
			}
			sonnet_map<uint32, Tracked> copy{ source };
			if (Tracked::alive != 40) {
				return false;
			}
		}
		return Tracked::alive == 0;
	}));

	tally(unit_test<"sonnet: move construction transfers the contents", true, false>::assert_eq(true, [] {
		sonnet_map<uint32, uint32> source{};
		for (uint32 i = 0; i < 30; ++i) {
			source.insert(pair<uint32, uint32>{ i, i * 2U });
		}
		sonnet_map<uint32, uint32> moved{ std::move(source) };
		if (moved.size() != 30) {
			return false;
		}
		for (uint32 i = 0; i < 30; ++i) {
			if (moved[i] != i * 2U) {
				return false;
			}
		}
		return true;
	}));

	tally(unit_test<"sonnet: move construction empties the source", true, false>::assert_eq(true, [] {
		sonnet_map<uint32, uint32> source{};
		for (uint32 i = 0; i < 30; ++i) {
			source.insert(pair<uint32, uint32>{ i, i });
		}
		sonnet_map<uint32, uint32> moved{ std::move(source) };
		return source.size() == 0 && moved.size() == 30;
	}));

	tally(unit_test<"sonnet: move assignment transfers the contents", true, false>::assert_eq(true, [] {
		sonnet_map<uint32, uint32> source{};
		for (uint32 i = 0; i < 30; ++i) {
			source.insert(pair<uint32, uint32>{ i, i * 2U });
		}
		sonnet_map<uint32, uint32> destination{};
		destination = std::move(source);
		return destination.size() == 30 && destination[15U] == 30U && source.size() == 0;
	}));

	tally(unit_test<"sonnet: move assignment discards prior destination contents", true, false>::assert_eq(true, [] {
		sonnet_map<uint32, uint32> source{};
		source.insert(pair<uint32, uint32>{ 1U, 1U });
		sonnet_map<uint32, uint32> destination{};
		for (uint32 i = 100; i < 120; ++i) {
			destination.insert(pair<uint32, uint32>{ i, i });
		}
		destination = std::move(source);
		return destination.size() == 1 && !destination.contains(100U);
	}));

	tally(unit_test<"sonnet: moved from map is reusable after insertion", true, false>::assert_eq(true, [] {
		sonnet_map<uint32, uint32> source{};
		for (uint32 i = 0; i < 10; ++i) {
			source.insert(pair<uint32, uint32>{ i, i });
		}
		sonnet_map<uint32, uint32> moved{ std::move(source) };
		source = sonnet_map<uint32, uint32>{};
		source.insert(pair<uint32, uint32>{ 5U, 50U });
		return source.size() == 1 && source[5U] == 50U && moved.size() == 10;
	}));

	tally(unit_test<"sonnet: move does not leak tracked values", true, false>::assert_eq(true, [] {
		Tracked::alive = 0;
		{
			sonnet_map<uint32, Tracked> source{};
			for (uint32 i = 0; i < 20; ++i) {
				source.insert(pair<uint32, Tracked>{ i, Tracked{ i } });
			}
			sonnet_map<uint32, Tracked> moved{ std::move(source) };
			if (Tracked::alive != 20) {
				return false;
			}
		}
		return Tracked::alive == 0;
	}));

	tally(unit_test<"sonnet: rehash to a larger capacity preserves every entry", true, false>::assert_eq(true, [] {
		sonnet_map<uint32, uint32> map{};
		for (uint32 i = 0; i < 50; ++i) {
			map.insert(pair<uint32, uint32>{ i, i * 4U });
		}
		map.rehash(4096);
		if (map.size() != 50 || map.capacity() < 4096) {
			return false;
		}
		for (uint32 i = 0; i < 50; ++i) {
			if (!map.contains(i) || map[i] != i * 4U) {
				return false;
			}
		}
		return true;
	}));

	tally(unit_test<"sonnet: rehash to a smaller capacity preserves every entry", true, false>::assert_eq(true, [] {
		sonnet_map<uint32, uint32> map{};
		for (uint32 i = 0; i < 20; ++i) {
			map.insert(pair<uint32, uint32>{ i, i * 4U });
		}
		map.rehash(4096);
		map.rehash(1);
		if (map.size() != 20) {
			return false;
		}
		for (uint32 i = 0; i < 20; ++i) {
			if (!map.contains(i) || map[i] != i * 4U) {
				return false;
			}
		}
		return true;
	}));

	tally(unit_test<"sonnet: rehash on an empty map leaves it empty", true, false>::assert_eq(true, [] {
		sonnet_map<uint32, uint32> map{};
		map.rehash(1024);
		return map.size() == 0 && map.begin() == map.end();
	}));

	tally(unit_test<"sonnet: insert after rehash continues to work", true, false>::assert_eq(true, [] {
		sonnet_map<uint32, uint32> map{};
		for (uint32 i = 0; i < 20; ++i) {
			map.insert(pair<uint32, uint32>{ i, i });
		}
		map.rehash(2048);
		for (uint32 i = 20; i < 40; ++i) {
			map.insert(pair<uint32, uint32>{ i, i });
		}
		if (map.size() != 40) {
			return false;
		}
		for (uint32 i = 0; i < 40; ++i) {
			if (!map.contains(i)) {
				return false;
			}
		}
		return true;
	}));

	tally(unit_test<"sonnet: rehash does not leak tracked values", true, false>::assert_eq(true, [] {
		Tracked::alive = 0;
		{
			sonnet_map<uint32, Tracked> map{};
			for (uint32 i = 0; i < 30; ++i) {
				map.insert(pair<uint32, Tracked>{ i, Tracked{ i } });
			}
			map.rehash(2048);
			if (Tracked::alive != 30) {
				return false;
			}
		}
		return Tracked::alive == 0;
	}));

	tally(unit_test<"sonnet: sixty four bit keys round trip", true, false>::assert_eq(true, [] {
		sonnet_map<uint64, uint64> map{};
		for (uint64 i = 0; i < 200; ++i) {
			map.insert(pair<uint64, uint64>{ i * 0x100000001ULL, i });
		}
		for (uint64 i = 0; i < 200; ++i) {
			if (map[i * 0x100000001ULL] != i) {
				return false;
			}
		}
		return map.size() == 200;
	}));

	tally(unit_test<"sonnet: short string keys round trip", true, false>::assert_eq(true, [] {
		sonnet_map<std::string, uint32> map{};
		for (uint32 i = 0; i < 200; ++i) {
			map.insert(pair<std::string, uint32>{ std::string{ "k" } + std::to_string(i), i });
		}
		for (uint32 i = 0; i < 200; ++i) {
			if (map[std::string{ "k" } + std::to_string(i)] != i) {
				return false;
			}
		}
		return map.size() == 200;
	}));

	tally(unit_test<"sonnet: long string keys beyond small string optimisation round trip", true, false>::assert_eq(true, [] {
		sonnet_map<std::string, uint32> map{};
		const std::string prefix{ "a-deliberately-long-key-well-past-any-sso-buffer-" };
		for (uint32 i = 0; i < 200; ++i) {
			map.insert(pair<std::string, uint32>{ prefix + std::to_string(i), i });
		}
		for (uint32 i = 0; i < 200; ++i) {
			if (map[prefix + std::to_string(i)] != i) {
				return false;
			}
		}
		return map.size() == 200;
	}));

	tally(unit_test<"sonnet: string keys of mixed length coexist", true, false>::assert_eq(true, [] {
		sonnet_map<std::string, uint32> map{};
		std::vector<std::string> keys{};
		for (uint32 i = 0; i < 100; ++i) {
			keys.emplace_back(std::string(static_cast<size_t>(1 + (i % 60)), static_cast<char>('a' + (i % 26))) + std::to_string(i));
		}
		for (uint32 i = 0; i < keys.size(); ++i) {
			map.insert(pair<std::string, uint32>{ keys[i], i });
		}
		if (map.size() != keys.size()) {
			return false;
		}
		for (uint32 i = 0; i < keys.size(); ++i) {
			if (map[keys[i]] != i) {
				return false;
			}
		}
		return true;
	}));

	tally(unit_test<"sonnet: empty string key is usable", true, false>::assert_eq(true, [] {
		sonnet_map<std::string, uint32> map{};
		map.insert(pair<std::string, uint32>{ std::string{}, 7U });
		return map.size() == 1 && map.contains(std::string{}) && map[std::string{}] == 7U;
	}));

	tally(unit_test<"sonnet: string keys erase correctly", true, false>::assert_eq(true, [] {
		sonnet_map<std::string, uint32> map{};
		for (uint32 i = 0; i < 50; ++i) {
			map.insert(pair<std::string, uint32>{ std::string{ "key-" } + std::to_string(i), i });
		}
		for (uint32 i = 0; i < 50; i += 2) {
			if (!map.erase(std::string{ "key-" } + std::to_string(i))) {
				return false;
			}
		}
		return map.size() == 25 && !map.contains(std::string{ "key-0" }) && map.contains(std::string{ "key-1" });
	}));

	tally(unit_test<"sonnet: std::string keys round trip", true, false>::assert_eq(true, [] {
		sonnet_map<uint32, std::string> map{};
		for (uint32 i = 0; i < 50; ++i) {
			map[i] = std::string{ "value" };
		}
		return map.size() == 50 && map[10U] == "value";
	}));

	tally(unit_test<"sonnet: move only values are supported", true, false>::assert_eq(true, [] {
		sonnet_map<uint32, std::unique_ptr<uint32>> map{};
		for (uint32 i = 0; i < 20; ++i) {
			map.insert(pair<uint32, std::unique_ptr<uint32>>{ i, std::make_unique<uint32>(i * 5U) });
		}
		if (map.size() != 20) {
			return false;
		}
		for (uint32 i = 0; i < 20; ++i) {
			auto it = map.find(i);
			if (it == map.end() || *(*it).second != i * 5U) {
				return false;
			}
		}
		return true;
	}));

	tally(unit_test<"sonnet: power of two keys do not collide destructively", true, false>::assert_eq(true, [] {
		sonnet_map<uint32, uint32> map{};
		for (uint32 i = 0; i < 32; ++i) {
			map.insert(pair<uint32, uint32>{ 1U << i, i });
		}
		if (map.size() != 32) {
			return false;
		}
		for (uint32 i = 0; i < 32; ++i) {
			if (map[1U << i] != i) {
				return false;
			}
		}
		return true;
	}));

	tally(unit_test<"sonnet: keys spaced by the bucket size survive probing", true, false>::assert_eq(true, [] {
		sonnet_map<uint32, uint32> map{};
		for (uint32 i = 0; i < 200; ++i) {
			map.insert(pair<uint32, uint32>{ i * 64U, i });
		}
		if (map.size() != 200) {
			return false;
		}
		for (uint32 i = 0; i < 200; ++i) {
			if (map[i * 64U] != i) {
				return false;
			}
		}
		return true;
	}));

	tally(unit_test<"sonnet: ten thousand entries remain individually addressable", true, false>::assert_eq(true, [] {
		sonnet_map<uint32, uint32> map{};
		for (uint32 i = 0; i < 10000; ++i) {
			map.insert(pair<uint32, uint32>{ i, i ^ 0x5A5A5A5AU });
		}
		if (map.size() != 10000) {
			return false;
		}
		for (uint32 i = 0; i < 10000; ++i) {
			if (map[i] != (i ^ 0x5A5A5A5AU)) {
				return false;
			}
		}
		return true;
	}));

	tally(unit_test<"sonnet: interleaved insert and erase tracks std::unordered_map", true, false>::assert_eq(true, [] {
		std::mt19937 rng{ 20260920U };
		std::unordered_map<uint32, uint32> oracle{};
		sonnet_map<uint32, uint32> map{};
		for (uint32 step = 0; step < 20000; ++step) {
			const uint32 key = rng() % 500U;
			const uint32 op	 = rng() % 3U;
			if (op == 0) {
				const uint32 value = rng();
				oracle.insert({ key, value });
				map.insert(pair<uint32, uint32>{ key, value });
			} else if (op == 1) {
				const uint32 value = rng();
				oracle[key]		   = value;
				map[key]		   = value;
			} else {
				const bool a = oracle.erase(key) != 0;
				const bool b = map.erase(key);
				if (a != b) {
					return false;
				}
			}
			if (oracle.size() != map.size()) {
				return false;
			}
		}
		for (auto& [key, value]: oracle) {
			if (!map.contains(key) || map[key] != value) {
				return false;
			}
		}
		uint64 seen = 0;
		for (auto& [key, value]: map) {
			auto it = oracle.find(key);
			if (it == oracle.end() || it->second != value) {
				return false;
			}
			++seen;
		}
		return seen == oracle.size();
	}));

	tally(unit_test<"sonnet: randomised string workload tracks std::unordered_map", true, false>::assert_eq(true, [] {
		std::mt19937 rng{ 7777U };
		std::unordered_map<std::string, uint32> oracle{};
		sonnet_map<std::string, uint32> map{};
		for (uint32 step = 0; step < 8000; ++step) {
			const std::string key{ std::string{ "s" } + std::to_string(rng() % 400U) };
			const uint32 op = rng() % 3U;
			if (op == 0) {
				const uint32 value = rng();
				oracle.insert({ key, value });
				map.insert(pair<std::string, uint32>{ key, value });
			} else if (op == 1) {
				const uint32 value = rng();
				oracle[key]		   = value;
				map[key]		   = value;
			} else {
				const bool a = oracle.erase(key) != 0;
				const bool b = map.erase(key);
				if (a != b) {
					return false;
				}
			}
			if (oracle.size() != map.size()) {
				return false;
			}
		}
		for (auto& [key, value]: oracle) {
			if (!map.contains(key) || map[key] != value) {
				return false;
			}
		}
		return true;
	}));

	tally(unit_test<"sonnet: erase heavy workload leaves a consistent map", true, false>::assert_eq(true, [] {
		sonnet_map<uint32, uint32> map{};
		for (uint32 i = 0; i < 2000; ++i) {
			map.insert(pair<uint32, uint32>{ i, i });
		}
		for (uint32 i = 0; i < 2000; ++i) {
			if ((i % 3) != 0) {
				map.erase(i);
			}
		}
		uint64 expected = 0;
		for (uint32 i = 0; i < 2000; ++i) {
			if ((i % 3) == 0) {
				++expected;
			}
		}
		if (map.size() != expected) {
			return false;
		}
		uint64 seen = 0;
		for (auto& [key, value]: map) {
			if ((key % 3) != 0 || value != key) {
				return false;
			}
			++seen;
		}
		return seen == expected;
	}));


	tally(unit_test<"sonnet: insert into exhausted probe sequence sets index", true, false>::assert_eq(true, [] {
		sonnet_map<uint32, uint32> map{ 4 };
		for (uint32 i = 0; i < 64; ++i) {
			map.insert(pair<uint32, uint32>{ i, i });
		}
		return map.size() == 64 && map.contains(63U);
	}));

	tally(unit_test<"sonnet: operator bracket does not grow on existing key", true, false>::assert_eq(true, [] {
		sonnet_map<uint32, uint32> map{ 16 };
		map.insert(pair<uint32, uint32>{ 5U, 50U });
		auto before = map.capacity();
		for (uint32 i = 0; i < 1000; ++i) {
			(nothing) map[5U];
		}
		return map.capacity() == before;
	}));

	tally(unit_test<"sonnet: moved from map stays usable", true, false>::assert_eq(true, [] {
		sonnet_map<uint32, uint32> source{};
		source[1U] = 10U;
		sonnet_map<uint32, uint32> target{ std::move(source) };
		const bool queried{ !source.contains(1U) && source.find(1U) == source.end() && source.empty() };
		source[2U] = 20U;
		return queried && source[2U] == 20U && target[1U] == 10U && !target.contains(2U);
	}));

	tally(unit_test<"sonnet: empty map answers every query", true, false>::assert_eq(true, [] {
		sonnet_map<uint32, uint32> map{};
		map.clear();
		return map.empty() && map.count(1U) == 0 && !map.erase(1U) && map.cbegin() == map.cend() && map.total_memory_usage() == sizeof(map);
	}));

	tally(unit_test<"sonnet: reserve prevents growth", true, false>::assert_eq(true, [] {
		sonnet_map<uint32, uint32> map{};
		map.reserve(1000);
		const uint64 reserved = map.capacity();
		for (uint32 i = 0; i < 1000; ++i) {
			map[i] = i;
		}
		return reserved >= 1000 && map.capacity() == reserved && map.size() == 1000;
	}));

	tally(unit_test<"sonnet: reserve keeps existing entries", true, false>::assert_eq(true, [] {
		sonnet_map<uint32, uint32> map{};
		map[5U] = 50U;
		map.reserve(5000);
		return map.size() == 1 && map[5U] == 50U;
	}));

	tally(unit_test<"sonnet: at returns and throws", true, false>::assert_eq(true, [] {
		sonnet_map<uint32, uint32> map{};
		map[1U]								   = 10U;
		map.at(1U)							   = 11U;
		const sonnet_map<uint32, uint32>& view = map;
		bool threw							   = false;
		try {
			static_cast<void>(view.at(2U));
		} catch (const std::out_of_range&) {
			threw = true;
		}
		return threw && view.at(1U) == 11U && map.size() == 1;
	}));

	tally(unit_test<"sonnet: count reports presence", true, false>::assert_eq(true, [] {
		sonnet_map<uint32, uint32> map{};
		map[1U] = 1U;
		return map.count(1U) == 1 && map.count(2U) == 0;
	}));

	tally(unit_test<"sonnet: insert returns iterator and inserted flag", true, false>::assert_eq(true, [] {
		sonnet_map<uint32, uint32> map{};
		auto [first, added]		   = map.insert(1U, 10U);
		auto [second, added_again] = map.insert(pair<uint32, uint32>{ 1U, 99U });
		return added && !added_again && first == second && second->second == 10U;
	}));

	tally(unit_test<"sonnet: emplace builds and deduplicates", true, false>::assert_eq(true, [] {
		sonnet_map<uint32, std::string> map{};
		const auto first  = map.emplace(1U, "one");
		const auto second = map.emplace(1U, "uno");
		return first.second && !second.second && map[1U] == "one";
	}));

	tally(unit_test<"sonnet: try_emplace leaves arguments alone on a hit", true, false>::assert_eq(true, [] {
		sonnet_map<uint32, std::unique_ptr<uint32>> map{};
		auto value			   = std::make_unique<uint32>(1U);
		const bool added	   = map.try_emplace(1U, std::move(value)).second;
		auto other			   = std::make_unique<uint32>(2U);
		const bool added_again = map.try_emplace(1U, std::move(other)).second;
		return added && !added_again && other != nullptr && *map[1U] == 1U;
	}));

	tally(unit_test<"sonnet: try_emplace forwards constructor arguments", true, false>::assert_eq(true, [] {
		sonnet_map<uint32, std::string> map{};
		map.try_emplace(1U, 3U, 'x');
		return map[1U] == "xxx";
	}));

	tally(unit_test<"sonnet: insert_or_assign overwrites", true, false>::assert_eq(true, [] {
		sonnet_map<uint32, uint32> map{};
		const bool added  = map.insert_or_assign(1U, 10U).second;
		const auto result = map.insert_or_assign(1U, 20U);
		return added && !result.second && result.first->second == 20U && map.size() == 1;
	}));

	tally(unit_test<"sonnet: erase accepts a temporary iterator", true, false>::assert_eq(true, [] {
		sonnet_map<uint32, uint32> map{};
		map[1U] = 1U;
		return map.erase(map.find(1U)) && map.empty();
	}));

	tally(unit_test<"sonnet: erase accepts a const iterator", true, false>::assert_eq(true, [] {
		sonnet_map<uint32, uint32> map{};
		map[1U]												  = 1U;
		sonnet_map<uint32, uint32>::const_iterator_type found = map.find(1U);
		return map.erase(found) && map.empty();
	}));

	tally(unit_test<"sonnet: erase range removes everything between", true, false>::assert_eq(true, [] {
		sonnet_map<uint32, uint32> map{};
		for (uint32 i = 0; i < 100; ++i) {
			map[i] = i;
		}
		const auto last = map.erase(map.begin(), map.end());
		return last == map.end() && map.empty();
	}));

	tally(unit_test<"sonnet: erase_if removes only matches", true, false>::assert_eq(true, [] {
		sonnet_map<uint32, uint32> map{};
		for (uint32 i = 0; i < 100; ++i) {
			map[i] = i * 2U;
		}
		const uint64 erased = void_containers::erase_if(map, [](const pair<uint32, uint32>& entry) {
			return entry.first < 25U;
		});
		return erased == 25 && map.size() == 75 && !map.contains(24U) && map[25U] == 50U;
	}));

	tally(unit_test<"sonnet: swap exchanges contents", true, false>::assert_eq(true, [] {
		sonnet_map<uint32, uint32> first{};
		sonnet_map<uint32, uint32> second{};
		first[1U]  = 10U;
		second[2U] = 20U;
		second[3U] = 30U;
		first.swap(second);
		return first.size() == 2 && first[2U] == 20U && second.size() == 1 && second[1U] == 10U;
	}));

	tally(unit_test<"sonnet: initializer list keeps first duplicate", true, false>::assert_eq(true, [] {
		sonnet_map<uint32, std::string> map{ { 1U, "one" }, { 2U, "two" }, { 1U, "uno" } };
		return map.size() == 2 && map[1U] == "one" && map[2U] == "two";
	}));

	tally(unit_test<"sonnet: range construction and insertion", true, false>::assert_eq(true, [] {
		const std::vector<std::pair<uint32, uint32>> values{ { 1U, 10U }, { 2U, 20U } };
		sonnet_map<uint32, uint32> map(values.begin(), values.end());
		const std::vector<std::pair<uint32, uint32>> more{ { 2U, 99U }, { 3U, 30U } };
		map.insert(more.begin(), more.end());
		map.insert({ { 4U, 40U } });
		return map.size() == 4 && map[2U] == 20U && map[3U] == 30U && map[4U] == 40U;
	}));

	tally(unit_test<"sonnet: equality compares keys and values", true, false>::assert_eq(true, [] {
		sonnet_map<uint32, uint32> first{};
		sonnet_map<uint32, uint32> second(4096);
		for (uint32 i = 0; i < 100; ++i) {
			first[i]		= i;
			second[99U - i] = 99U - i;
		}
		const bool equal		 = first == second;
		second[5U]				 = 500U;
		const bool value_differs = first != second;
		second[5U]				 = 5U;
		second.erase(6U);
		return equal && value_differs && first != second;
	}));

	tally(unit_test<"sonnet: iterator converts and compares with const iterator", true, false>::assert_eq(true, [] {
		sonnet_map<uint32, uint32> map{};
		map[1U]													 = 10U;
		auto mutable_it											 = map.find(1U);
		sonnet_map<uint32, uint32>::const_iterator_type const_it = mutable_it;
		return const_it == mutable_it && mutable_it == const_it && const_it->second == 10U && map.cbegin() == const_it;
	}));

	tally(unit_test<"sonnet: string literals look up and insert directly", true, false>::assert_eq(true, [] {
		sonnet_map<std::string, uint32> map{};
		map["one"] = 1U;
		map.insert("two", 2U);
		const char* pointer			= "two";
		const std::string_view view = "one";
		return map.contains("one") && map.at(pointer) == 2U && map.find(view)->second == 1U && map.count("three") == 0 && map.erase("one") && map.size() == 1;
	}));

	tally(unit_test<"sonnet: custom equality and hash fold keys", true, false>::assert_eq(true, [] {
		sonnet_map<std::string, uint32, case_blind_hasher, case_blind_equal> map{};
		map[std::string{ "KEY" }] = 1U;
		map[std::string{ "key" }] = 2U;
		return map.size() == 1 && map[std::string{ "Key" }] == 2U;
	}));

	tally(unit_test<"sonnet: move only keys survive growth", true, false>::assert_eq(true, [] {
		sonnet_map<move_only_key, uint32, move_only_hasher> map{};
		for (uint32 i = 0; i < 1000; ++i) {
			map.insert(move_only_key{ std::make_unique<uint32>(i) }, i);
		}
		const move_only_key probe{ std::make_unique<uint32>(999U) };
		return map.size() == 1000 && map.at(probe) == 999U;
	}));

	std::cout << "==== sonnet_map suite: " << (failure_count == 0 ? "all tests passed" : "failures detected") << " (" << failure_count << " failing) ====" << std::endl;

	return failure_count == 0 ? 0 : 1;
}
