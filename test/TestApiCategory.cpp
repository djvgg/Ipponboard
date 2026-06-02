// Copyright 2018 Florian Muecke. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE.txt file.

#include "../util/catch2/catch.hpp"
#include "../api/ApiEndpoints.h"

using namespace Ipponboard;

TEST_CASE("[api] gender label normalization maps to M/F")
{
	// Male variants (English / German, any case)
	CHECK(NormalizeGenderLabel("m") == "M");
	CHECK(NormalizeGenderLabel("M") == "M");
	CHECK(NormalizeGenderLabel("male") == "M");
	CHECK(NormalizeGenderLabel("Male") == "M");
	CHECK(NormalizeGenderLabel(" male ") == "M");

	// Female variants — note JF/edv upstream uses "female"/"f", never "w",
	// but German "weiblich" must map too.
	CHECK(NormalizeGenderLabel("f") == "F");
	CHECK(NormalizeGenderLabel("F") == "F");
	CHECK(NormalizeGenderLabel("female") == "F");
	CHECK(NormalizeGenderLabel("Female") == "F");
	CHECK(NormalizeGenderLabel("w") == "F");
	CHECK(NormalizeGenderLabel("weiblich") == "F");
}

TEST_CASE("[api] unknown / empty gender passes through")
{
	CHECK(NormalizeGenderLabel("") == "");
	CHECK(NormalizeGenderLabel("   ") == "");
	CHECK(NormalizeGenderLabel("x") == "x");
}

TEST_CASE("[api] BuildCategoryName matches Ipponboard catalogue naming")
{
	// These are the canonical FightCategory names in FightCategoryManager.cpp
	CHECK(BuildCategoryName("m", "U15") == "MU15");
	CHECK(BuildCategoryName("male", "U18") == "MU18");
	CHECK(BuildCategoryName("female", "U18") == "FU18");
	CHECK(BuildCategoryName("w", "U13") == "FU13");
	CHECK(BuildCategoryName("m", "18+") == "M18+");

	// Trims the age-group too
	CHECK(BuildCategoryName("m", " U15 ") == "MU15");

	// Gender-neutral age groups (U9/U11) carry no catalogue gender prefix; a
	// caller that omits gender resolves to the bare name.
	CHECK(BuildCategoryName("", "U11") == "U11");
}
