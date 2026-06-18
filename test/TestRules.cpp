// Copyright 2018 Florian Muecke. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE.txt file.

#include "../util/catch2/catch.hpp"
#include "../core/Score.h"
#include "../core/Enums.h"
#include "../core/Rules.h"
//#include "../core/Fight.h"

#include <iostream>

using namespace Ipponboard;
using Point = Score::Point;

TEST_CASE("[Rules] Rulesfactory creates correct rule object")
{
	auto rules = RulesFactory::Create(ClassicRules::StaticName);
	REQUIRE(strcmp(rules->Name(), ClassicRules::StaticName) == 0);

	rules = RulesFactory::Create(Rules2013::StaticName);
	REQUIRE(strcmp(rules->Name(), Rules2013::StaticName) == 0);

	rules = RulesFactory::Create(Rules2017U15::StaticName);
	REQUIRE(strcmp(rules->Name(), Rules2017U15::StaticName) == 0);

	rules = RulesFactory::Create(Rules2017::StaticName);
	REQUIRE(strcmp(rules->Name(), Rules2017::StaticName) == 0);

	rules = RulesFactory::Create(Rules2018::StaticName);
	REQUIRE(strcmp(rules->Name(), Rules2018::StaticName) == 0);

	rules = RulesFactory::Create(Rules2025::StaticName);
	REQUIRE(strcmp(rules->Name(), Rules2025::StaticName) == 0);

	rules = RulesFactory::Create(RulesPfalzU13::StaticName);
	REQUIRE(strcmp(rules->Name(), RulesPfalzU13::StaticName) == 0);
}

TEST_CASE("[Rules] Default rules are IJF-2025")
{
	auto rules = RulesFactory::Create("");
	REQUIRE(strcmp(rules->Name(), Rules2025::StaticName) == 0);
}

TEST_CASE("[Rules] String list contains all rules")
{
	auto names = RulesFactory::GetNames();
	REQUIRE(names.contains(ClassicRules::StaticName));
	REQUIRE(names.contains(Rules2013::StaticName));
	REQUIRE(names.contains(Rules2017U15::StaticName));
	REQUIRE(names.contains(Rules2017::StaticName));
	REQUIRE(names.contains(Rules2018::StaticName));
	REQUIRE(names.contains(Rules2025::StaticName));
	REQUIRE(names.contains(RulesPfalzU13::StaticName));
	REQUIRE(names.size() == 7);
}

TEST_CASE("[Rules] 2025: osaekomi values")
{
	auto rules = std::make_shared<Rules2025>();
	
	INFO("ippon is gained after 20 seconds")
	REQUIRE(rules->GetOsaekomiValue(Score::Point::Ippon) == 20);

	INFO("wazaari is gained after 10 seconds")
	REQUIRE(rules->GetOsaekomiValue(Score::Point::Wazaari) == 10);

	INFO("yuko is gained after 5 seconds")
	REQUIRE(rules->GetOsaekomiValue(Score::Point::Yuko) == 5);
}

TEST_CASE("[Rules] JVP additive: osaekomi values")
{
	auto rules = std::make_shared<RulesPfalzU13>();

	INFO("ippon after 20 seconds")
	REQUIRE(rules->GetOsaekomiValue(Score::Point::Ippon) == 20);

	INFO("waza-ari after 10 seconds")
	REQUIRE(rules->GetOsaekomiValue(Score::Point::Wazaari) == 10);

	INFO("yuko after 5 seconds")
	REQUIRE(rules->GetOsaekomiValue(Score::Point::Yuko) == 5);
}

TEST_CASE("[Rules] JVP additive: point values 10/5/3/2")
{
	auto rules = std::make_shared<RulesPfalzU13>();

	REQUIRE(rules->GetPointValue(Score::Point::Ippon) == 10);
	REQUIRE(rules->GetPointValue(Score::Point::Wazaari) == 5);
	REQUIRE(rules->GetPointValue(Score::Point::Yuko) == 3);
	REQUIRE(rules->GetPointValue(Score::Point::Shido) == 2);

	INFO("no waza-ari-awasete-ippon in the additive system")
	REQUIRE_FALSE(rules->IsOption_AwaseteIppon());
}

TEST_CASE("[Rules] JVP additive: per-sequence cap is 10, IJF rulesets uncapped")
{
	REQUIRE(RulesPfalzU13().GetMaxSequencePoints() == 10);

	INFO("every IJF ruleset stays uncapped (no behaviour change)")
	REQUIRE(Rules2025().GetMaxSequencePoints() == INT32_MAX);
	REQUIRE(ClassicRules().GetMaxSequencePoints() == INT32_MAX);
}

TEST_CASE("[Rules] hold-time OOB guard: FighterEnum contract m_seqSnapshot relies on")
{
	// Controller::update_hold_time + IpponboardSM_::add_point(HoldTimeEvent) guard on
	// "tori is a real fighter" before indexing m_seqSnapshot[2] (JVP sequence cap).
	// That guard is only correct if Nobody is OUTSIDE {0,1} and First/Second ARE the
	// valid indices. If the enum is ever renumbered, the guard must be revisited —
	// pin the contract here so the OOB (m_seqSnapshot[Nobody] == [-1], UB) can't
	// silently come back. See outbox/wsp-debug-2026-06-18.md.
	REQUIRE(static_cast<int>(FighterEnum::First) == 0);
	REQUIRE(static_cast<int>(FighterEnum::Second) == 1);
	REQUIRE(static_cast<int>(FighterEnum::Nobody) < 0);

	INFO("Nobody must not be a valid m_seqSnapshot index")
	const int nobody = static_cast<int>(FighterEnum::Nobody);
	REQUIRE((nobody < 0 || nobody > 1));
}

TEST_CASE("[Rules] additive max counts are INT32_MAX (cap+1 would overflow)")
{
	// The revoke-shido (StateMachine) and hansokumake-render (View) paths test
	// "fighter is one beyond the shido cap". They are written as Shido()-1 == cap,
	// NOT cap+1 == Shido(), because for the additive ruleset cap is INT32_MAX and
	// cap+1 is signed overflow (UB). Pin the precondition + the equivalence.
	REQUIRE(RulesPfalzU13().GetMaxShidoCount() == INT32_MAX);
	REQUIRE(RulesPfalzU13().GetMaxIpponCount() == INT32_MAX);
	REQUIRE(RulesPfalzU13().GetMaxWazaariCount() == INT32_MAX);

	// Shido()-1 == cap is overflow-free and matches cap+1 == Shido() for every
	// realistic (small, >=0) shido count under a capped IJF ruleset.
	const int cap = Rules2025().GetMaxShidoCount(); // 3
	for (int shido = 0; shido <= 6; ++shido)
	{
		const bool viaSub = (shido - 1 == cap);          // the form we ship
		const bool viaAdd = (cap + 1 == shido);          // the old form (cap small here)
		REQUIRE(viaSub == viaAdd);
	}

	INFO("for the uncapped additive ruleset the branch is simply never true")
	const int additiveCap = RulesPfalzU13().GetMaxShidoCount(); // INT32_MAX
	for (int shido = 0; shido <= 50; ++shido)
		REQUIRE_FALSE(shido - 1 == additiveCap);
}

TEST_CASE("[Rules] additive Hansoku-make awards no Ippon; IJF does")
{
	// Drives the StateMachine gate: under IJF a direct Hansoku-make hands the
	// opponent a full Ippon (= win), but in the JVP additive system it is a pure
	// DQ (WKO §3.5.2) — adding an Ippon would inflate the opponent's 0..20 total.
	REQUIRE_FALSE(RulesPfalzU13().IsOption_HansokumakeAwardsIppon());

	INFO("IJF rulesets keep the Ippon-on-HM behaviour")
	REQUIRE(Rules2025().IsOption_HansokumakeAwardsIppon());
	REQUIRE(ClassicRules().IsOption_HansokumakeAwardsIppon());
}

TEST_CASE("[Rules] Shido is uncapped in the additive system, <=3 under IJF")
{
	// The operator console renders a Shido *counter* (digit) instead of the 3 IJF
	// lamps exactly when GetMaxShidoCount() > 3 — 3 lamps cannot show 4+ Shido.
	// This is the predicate View::update_shido() keys on, so guard it here.
	REQUIRE(RulesPfalzU13().GetMaxShidoCount() > 3);

	INFO("IJF rulesets stay within the 3-lamp range (lamp display unchanged)")
	REQUIRE(Rules2025().GetMaxShidoCount() <= 3);
	REQUIRE(ClassicRules().GetMaxShidoCount() <= 3);
}