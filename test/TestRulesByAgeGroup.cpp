// Copyright 2018 Florian Muecke. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE.txt file.

#include "../util/catch2/catch.hpp"
#include "../base/RulesByAgeGroup.h"

using namespace Ipponboard;

TEST_CASE("[rulesbyage] age group derived from resolved category")
{
	CHECK(AgeGroupFromCategory("MU15") == "U15");
	CHECK(AgeGroupFromCategory("FU18") == "U18");
	CHECK(AgeGroupFromCategory("M18+") == "18+");
	CHECK(AgeGroupFromCategory("U11") == "U11");   // no gender prefix
	CHECK(AgeGroupFromCategory("U9") == "U9");
	CHECK(AgeGroupFromCategory(" MU13 ") == "U13"); // trimmed
	CHECK(AgeGroupFromCategory("") == "");
}

TEST_CASE("[rulesbyage] spec parsing")
{
	auto map = ParseRulesByAgeGroup("U9:JVP-Additiv-20;U11:JVP-Additiv-20");
	REQUIRE(map.size() == 2);
	CHECK(map[0].first == "U9");
	CHECK(map[0].second == "JVP-Additiv-20");   // hyphenated ruleset name kept intact
	CHECK(map[1].first == "U11");
	CHECK(map[1].second == "JVP-Additiv-20");

	// malformed entries skipped, whitespace trimmed
	auto messy = ParseRulesByAgeGroup(" U9 : JVP-Additiv-20 ;; nogarbage ; U11:IJF-2025");
	REQUIRE(messy.size() == 2);
	CHECK(messy[0].first == "U9");
	CHECK(messy[1].first == "U11");
	CHECK(messy[1].second == "IJF-2025");

	CHECK(ParseRulesByAgeGroup("").empty());
}

TEST_CASE("[rulesbyage] ruleset selection with default fallback")
{
	const auto map = ParseRulesByAgeGroup(DefaultRulesByAgeGroupSpec());

	// listed age groups -> Pfalz additive
	CHECK(RulesetForAgeGroup(map, "U9", "IJF-2025") == "JVP-Additiv-20");
	CHECK(RulesetForAgeGroup(map, "U11", "IJF-2025") == "JVP-Additiv-20");
	CHECK(RulesetForAgeGroup(map, "u11", "IJF-2025") == "JVP-Additiv-20"); // case-insensitive

	// unlisted age groups -> default (IJF), NOT the previous fight's ruleset
	CHECK(RulesetForAgeGroup(map, "U13", "IJF-2025") == "IJF-2025");
	CHECK(RulesetForAgeGroup(map, "U15", "IJF-2025") == "IJF-2025");
	CHECK(RulesetForAgeGroup(map, "18+", "IJF-2025") == "IJF-2025");
}
