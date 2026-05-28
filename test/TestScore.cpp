// Copyright 2018 Florian Muecke. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE.txt file.

#include "../util/catch2/catch.hpp"
#include "../core/Score.h"
#include "../core/Enums.h"
#include "../core/Rules.h"
#include "../core/Fight.h"

#include <iostream>

using namespace Ipponboard;
using Point = Score::Point;

bool IsScoreLess(std::shared_ptr<Ipponboard::AbstractRules> pRules, Score const& lhs, Score const& rhs)
{
	Fight f { lhs, rhs };
	return pRules->CompareScore(f) > 0;
}

TEST_CASE("[Score] Shido rules for fights")
{
	auto empty = Score();
	auto shido = Score().Add(Point::Shido);
	auto twoShido = Score().Add(Point::Shido).Add(Point::Shido);
	auto yukoWithShido = Score().Add(Point::Yuko).Add(Point::Shido);
	auto yukoWithTwoShido = Score().Add(Point::Yuko).Add(Point::Shido).Add(Point::Shido);
	auto wazaari = Score().Add(Point::Wazaari);
	auto wazaariWithShido = Score().Add(Point::Wazaari).Add(Point::Shido);
	auto wazaariWithTwoShido = Score().Add(Point::Wazaari).Add(Point::Shido).Add(Point::Shido);
	auto wazaariWithThreeShido = Score().Add(Point::Wazaari).Add(Point::Shido).Add(Point::Shido).Add(Point::Shido);

	auto rules2013 = std::make_shared<Ipponboard::Rules2013>();
	REQUIRE_FALSE(IsScoreLess(rules2013, shido, shido));
	REQUIRE(IsScoreLess(rules2013, shido, empty));
	REQUIRE(IsScoreLess(rules2013, twoShido, shido));
	REQUIRE(IsScoreLess(rules2013, shido, yukoWithTwoShido));
	REQUIRE(IsScoreLess(rules2013, twoShido, yukoWithTwoShido));
	REQUIRE(IsScoreLess(rules2013, empty, yukoWithTwoShido));
	REQUIRE(IsScoreLess(rules2013, wazaariWithShido, wazaari));
	REQUIRE(IsScoreLess(rules2013, wazaariWithTwoShido, wazaari));
	REQUIRE(IsScoreLess(rules2013, wazaariWithTwoShido, wazaariWithShido));
	REQUIRE(IsScoreLess(rules2013, wazaariWithThreeShido, wazaariWithShido));
	REQUIRE(IsScoreLess(rules2013, wazaariWithThreeShido, wazaariWithTwoShido));

	// classic rules
	auto classicRules = std::make_shared<Ipponboard::ClassicRules>();
	REQUIRE_FALSE(IsScoreLess(classicRules, shido, empty));
	REQUIRE_FALSE(IsScoreLess(classicRules, shido, shido));
	REQUIRE(IsScoreLess(classicRules, twoShido, yukoWithShido));
	REQUIRE(IsScoreLess(classicRules, shido, yukoWithTwoShido));
	REQUIRE(IsScoreLess(classicRules, twoShido, yukoWithTwoShido));
	REQUIRE(IsScoreLess(classicRules, empty, yukoWithTwoShido));
}

//TEST_CASE("4th shido sets hansokumake")
//{
//	Score one;
//	Score two;
//
//	one.Add(Point::Shido);
//	one.Add(Point::Shido);
//	one.Add(Point::Shido);
//	one.Add(Point::Shido);
//	two.Add(Hansokumake);
//
//	one.Hansokumake
//	REQUIRE_FALSE(one.IsLess(two));
//	REQUIRE_FALSE(two.IsLess(one));
//}

TEST_CASE("[Score] Each fighter can have Hansokumake")
{
	Score score1;
	Score score2;
	auto classicRules = std::make_shared<Ipponboard::ClassicRules>();
	auto rules2013 = std::make_shared<Ipponboard::Rules2013>();

	score1.Add(Score::Point::Hansokumake);

	REQUIRE(IsScoreLess(classicRules, score1, score2));
	REQUIRE_FALSE(IsScoreLess(classicRules, score2, score1));

	REQUIRE(IsScoreLess(rules2013, score1, score2));
	REQUIRE_FALSE(IsScoreLess(rules2013, score2, score1));

	score2.Add(Score::Point::Hansokumake);

	REQUIRE_FALSE(IsScoreLess(classicRules, score1, score2));
	REQUIRE_FALSE(IsScoreLess(classicRules, score2, score1));

	REQUIRE_FALSE(IsScoreLess(rules2013, score1, score2));
	REQUIRE_FALSE(IsScoreLess(rules2013, score2, score1));
}

TEST_CASE("[Score] is awasette ippon")
{
	Score score;
	auto classicRules = std::make_shared<Ipponboard::ClassicRules>();
	auto rules2013 = std::make_shared<Ipponboard::Rules2013>();
	auto rules2017 = std::make_shared<Ipponboard::Rules2017>();
	auto rules2018 = std::make_shared<Ipponboard::Rules2018>();
	auto rules2025 = std::make_shared<Ipponboard::Rules2025>();

	REQUIRE_FALSE(classicRules->IsAwaseteIppon(score));
	REQUIRE_FALSE(rules2013->IsAwaseteIppon(score));
	REQUIRE_FALSE(rules2017->IsAwaseteIppon(score));
	REQUIRE_FALSE(rules2018->IsAwaseteIppon(score));
	REQUIRE_FALSE(rules2025->IsAwaseteIppon(score));

	score.Add(Score::Point::Wazaari);

	REQUIRE_FALSE(classicRules->IsAwaseteIppon(score));
	REQUIRE_FALSE(rules2013->IsAwaseteIppon(score));
	REQUIRE_FALSE(rules2017->IsAwaseteIppon(score));
	REQUIRE_FALSE(rules2018->IsAwaseteIppon(score));
	REQUIRE_FALSE(rules2025->IsAwaseteIppon(score));

	score.Add(Score::Point::Wazaari);

	REQUIRE(classicRules->IsAwaseteIppon(score));
	REQUIRE(rules2013->IsAwaseteIppon(score));
	REQUIRE_FALSE(rules2017->IsAwaseteIppon(score));
	REQUIRE(rules2018->IsAwaseteIppon(score));
	REQUIRE(rules2025->IsAwaseteIppon(score));

	//TODO
	//FIXME
	/* not correct, yet
	score.Add(Score::Point::Wazaari);

	std::cout << "ippon: " << score.Ippon() << ", ";
	std::cout << "wazaari: " << score.Wazaari();
	REQUIRE(classicRules->IsAwaseteIppon(score));
	REQUIRE(rules2013->IsAwaseteIppon(score));
	REQUIRE_FALSE(rules2017->IsAwaseteIppon(score));
	REQUIRE(rules2018->IsAwaseteIppon(score));
	*/
}

TEST_CASE("[Score] rules 2017: only first shido does not count")
{
	auto rules = std::make_shared<Ipponboard::Rules2017>();

	Score s1;
	REQUIRE_FALSE(IsScoreLess(rules, s1, Score()));

	s1.Add(Point::Shido);
	REQUIRE_FALSE(IsScoreLess(rules, s1, Score()));

	s1.Add(Point::Shido);
	REQUIRE_FALSE(IsScoreLess(rules, s1, Score()));  // as yuko has to be added manually
}

TEST_CASE("[Score] rules 2017: first shido does count in golden score")
{
	auto rules = std::make_shared<Ipponboard::Rules2017>();

	Fight f;
	f.SetGoldenScore(true);

	REQUIRE(rules->CompareScore(f) == 0);

	f.GetScore(FighterEnum::First).Add(Point::Shido);
	REQUIRE(rules->CompareScore(f) > 0);

	f.GetScore(FighterEnum::First).Add(Point::Shido);
	REQUIRE(rules->CompareScore(f) > 0);
}

TEST_CASE("[Score] rules 2025: yuko is always less than wazaari")
{
	auto rules = std::make_shared<Ipponboard::Rules2025>();

	Score yukoScore, wazaariScore; 
	yukoScore.Add(Point::Yuko);
	INFO("empty score is less than yuko");
	REQUIRE(IsScoreLess(rules, Score(), yukoScore));

	wazaariScore.Add(Point::Wazaari);
	INFO("yuko is less than wazaari");
	REQUIRE(IsScoreLess(rules, yukoScore, wazaariScore));

	yukoScore.Add(Point::Yuko);
	yukoScore.Add(Point::Yuko);
	INFO("3 yuko is less than wazaari");
	REQUIRE(IsScoreLess(rules, yukoScore, wazaariScore));

	for (int i = 0; i < 97; ++i)
	{
		yukoScore.Add(Point::Yuko);
	}
	INFO("100 yuko is less than wazaari");
	REQUIRE(IsScoreLess(rules, yukoScore, wazaariScore));
}

TEST_CASE("[Score] without a ruleset the historic caps still apply")
{
	Score s;
	s.SetValue(Point::Ippon, 2);
	INFO("ippon is clamped to 1 when no ruleset is attached (IJF default)");
	REQUIRE(s.Value(Point::Ippon) == 1);

	Score shido;
	for (int i = 0; i < 6; ++i) shido.Add(Point::Shido);
	INFO("shido is clamped to 4 when no ruleset is attached");
	REQUIRE(shido.Shido() == 4);
}

TEST_CASE("[Score] JVP additive: scores accumulate past the IJF caps")
{
	auto pfalz = std::make_shared<RulesPfalzU13>();

	Score twoIppon;
	twoIppon.SetRules(pfalz.get());
	twoIppon.SetValue(Point::Ippon, 2);
	INFO("two ippon are allowed in the additive system (10+10)");
	REQUIRE(twoIppon.Value(Point::Ippon) == 2);

	Score fourWaza;
	fourWaza.SetRules(pfalz.get());
	for (int i = 0; i < 4; ++i) fourWaza.Add(Point::Wazaari);
	INFO("four waza-ari are allowed (4x5 = 20)");
	REQUIRE(fourWaza.Wazaari() == 4);

	Score manyShido;
	manyShido.SetRules(pfalz.get());
	for (int i = 0; i < 6; ++i) manyShido.Add(Point::Shido);
	INFO("shido is uncapped in the additive system");
	REQUIRE(manyShido.Shido() == 6);
}

TEST_CASE("[Score] JVP additive: total and winner")
{
	auto pfalz = std::make_shared<RulesPfalzU13>();

	SECTION("sum = 10*ippon + 5*wazaari + 3*yuko")
	{
		Score s1;
		s1.SetRules(pfalz.get());
		s1.Add(Point::Wazaari).Add(Point::Yuko); // 5 + 3 = 8
		Score s2;
		s2.SetRules(pfalz.get());
		Fight f { s1, s2 };
		REQUIRE(pfalz->GetTotalScore(f, FighterEnum::First) == 8);
		REQUIRE(pfalz->GetTotalScore(f, FighterEnum::Second) == 0);
		REQUIRE(pfalz->CompareScore(f) < 0); // First leads
	}

	SECTION("each shido is worth +2 for the opponent")
	{
		Score s1;
		s1.SetRules(pfalz.get());
		Score s2;
		s2.SetRules(pfalz.get());
		s2.Add(Point::Shido).Add(Point::Shido); // second fighter penalised twice
		Fight f { s1, s2 };
		INFO("two shido of fighter 2 give fighter 1 four points");
		REQUIRE(pfalz->GetTotalScore(f, FighterEnum::First) == 4);
		REQUIRE(pfalz->GetTotalScore(f, FighterEnum::Second) == 0);
		REQUIRE(pfalz->CompareScore(f) < 0);
	}

	SECTION("reaching 20 wins (Sore Made), total clamps at 20")
	{
		Score s1;
		s1.SetRules(pfalz.get());
		s1.SetValue(Point::Ippon, 2); // 20
		Score s2;
		s2.SetRules(pfalz.get());
		s2.Add(Point::Wazaari); // 5
		Fight f { s1, s2 };
		REQUIRE(pfalz->GetTotalScore(f, FighterEnum::First) == 20);
		REQUIRE(pfalz->CompareScore(f) < 0);
	}

	SECTION("higher total wins, equal total is a Hiki-wake (draw)")
	{
		Score wazaari;
		wazaari.SetRules(pfalz.get());
		wazaari.Add(Point::Wazaari); // 5
		Score twoYuko;
		twoYuko.SetRules(pfalz.get());
		twoYuko.Add(Point::Yuko).Add(Point::Yuko); // 6
		Fight uneven { wazaari, twoYuko };
		REQUIRE(pfalz->CompareScore(uneven) > 0); // Second leads 6 vs 5

		Score a;
		a.SetRules(pfalz.get());
		a.Add(Point::Wazaari); // 5
		Score b;
		b.SetRules(pfalz.get());
		b.Add(Point::Wazaari); // 5
		Fight draw { a, b };
		REQUIRE(pfalz->CompareScore(draw) == 0);
	}

	SECTION("a direct hansoku-make decides regardless of points")
	{
		Score s1;
		s1.SetRules(pfalz.get());
		s1.SetValue(Point::Ippon, 1); // 10 points on the board
		s1.Add(Point::Hansokumake);   // but disqualified
		Score s2;
		s2.SetRules(pfalz.get());
		Fight f { s1, s2 };
		REQUIRE(pfalz->CompareScore(f) > 0); // Second wins despite trailing on points
	}
}

TEST_CASE("[Score] JVP additive: GetDisplayTotal mirrors total; IJF returns -1")
{
	auto pfalz = std::make_shared<RulesPfalzU13>();
	Score s1;
	s1.SetRules(pfalz.get());
	s1.Add(Point::Wazaari).Add(Point::Yuko); // 5 + 3 = 8
	Score s2;
	s2.SetRules(pfalz.get());
	Fight f { s1, s2 };

	REQUIRE(pfalz->GetDisplayTotal(f, FighterEnum::First) == 8);
	REQUIRE(pfalz->GetDisplayTotal(f, FighterEnum::Second) == 0);

	INFO("non-additive IJF ruleset signals 'no total' with -1");
	Rules2025 ijf;
	REQUIRE(ijf.GetDisplayTotal(f, FighterEnum::First) == -1);
}
