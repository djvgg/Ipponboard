// Copyright 2018 Florian Muecke. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE.txt file.

#include "Rules.h"
#include "Fight.h"

using namespace Ipponboard;

const char* const Rules2025::StaticName = "IJF-2025";
const char* const Rules2018::StaticName = "IJF-2018";
const char* const Rules2017::StaticName = "IJF-2017";
const char* const Rules2017U15::StaticName = "IJF-2017 U15";
const char* const Rules2013::StaticName = "IJF-2013";
const char* const ClassicRules::StaticName = "Classic";
const char* const RulesPfalzU13::StaticName = "JVP-Additiv-20";

AbstractRules::AbstractRules()
{}

int Ipponboard::AbstractRules::GetPointValue(Score::Point p) const
{
	// Default = the IJF team-list points (mirrors Fight::eScore_*).
	switch (p)
	{
	case Score::Point::Ippon: return 10;

	case Score::Point::Wazaari: return 7;

	case Score::Point::Yuko: return 5;

	case Score::Point::Shido: return 1;

	default: return 0;
	}
}

int Ipponboard::AbstractRules::CompareScore(const Fight& f) const
{
	using Point = Score::Point;

	auto lhs = f.GetScore(FighterEnum::First);
	auto rhs = f.GetScore(FighterEnum::Second);

	if (lhs.Value(Point::Hansokumake) < rhs.Value(Point::Hansokumake))
	{
		return -1;
	}

	if ((lhs.Value(Point::Hansokumake) > rhs.Value(Point::Hansokumake)))
	{
		return 1;
	}

	if (lhs.Value(Point::Ippon) > rhs.Value(Point::Ippon))
	{
		return -1;
	}

	if (lhs.Value(Point::Ippon) < rhs.Value(Point::Ippon))
	{
		return 1;
	}

	if (lhs.Value(Point::Wazaari) > rhs.Value(Point::Wazaari))
	{
		return -1;
	}

	if (lhs.Value(Point::Wazaari) < rhs.Value(Point::Wazaari))
	{
		return 1;
	}

	if (lhs.Value(Point::Yuko) > rhs.Value(Point::Yuko))
	{
		return -1;
	}

	if (lhs.Value(Point::Yuko) < rhs.Value(Point::Yuko))
	{
		return 1;
	}

	// shidos are not compared as they result in concrete points
	if (!IsOption_ShidoAddsPoint() && (IsOption_ShidoScoreCounts() || f.IsGoldenScore()))
	{
		if (lhs.Value(Point::Shido) < rhs.Value(Point::Shido))
		{
			return -1;
		}

		if (lhs.Value(Point::Shido) > rhs.Value(Point::Shido))
		{
			return 1;
		}
	}

	return 0;
}

int Ipponboard::RulesPfalzU13::GetTotalScore(const Fight& f, FighterEnum who) const
{
	using Point = Score::Point;

	const Score& own = f.GetScore(who);
	const Score& opp = f.GetScore(GetUkeFromTori(who));

	int total =
		own.Value(Point::Ippon) * GetPointValue(Point::Ippon)
		+ own.Wazaari() * GetPointValue(Point::Wazaari)
		+ own.Yuko() * GetPointValue(Point::Yuko)
		// each Shido of the opponent is worth +2 for me
		+ opp.Shido() * GetPointValue(Point::Shido);

	// at most 20 points are reachable (Sore Made)
	return total > 20 ? 20 : total;
}

int Ipponboard::RulesPfalzU13::GetDisplayTotal(const Fight& f, FighterEnum who) const
{
	return GetTotalScore(f, who);
}

int Ipponboard::RulesPfalzU13::CompareScore(const Fight& f) const
{
	const Score& lhs = f.GetScore(FighterEnum::First);
	const Score& rhs = f.GetScore(FighterEnum::Second);

	// A direct Hansoku-make decides outright (DQ), regardless of points.
	if (lhs.Hansokumake() != rhs.Hansokumake())
	{
		return lhs.Hansokumake() ? 1 : -1;
	}

	const int t1 = GetTotalScore(f, FighterEnum::First);
	const int t2 = GetTotalScore(f, FighterEnum::Second);

	if (t1 > t2)
	{
		return -1;
	}

	if (t1 < t2)
	{
		return 1;
	}

	return 0; // Hiki-wake
}
