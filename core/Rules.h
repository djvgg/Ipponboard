// Copyright 2018 Florian Muecke. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE.txt file.

#ifndef RULESET_H
#define RULESET_H

#include "Score.h"
#include <QString>
#include <QStringList>
#include <memory>

namespace Ipponboard
{
class Fight;
class AbstractRules
{
public:
	AbstractRules();

	virtual const char* Name() const = 0;

	virtual void SetAlwaysAutoAdjustPoints(bool autoAdjust)
	{
		_isAlwaysAutoAdjustPoints = autoAdjust;
	}

	virtual void SetCountSubscores(bool countSubscores)
	{
		_isCountSubscores = countSubscores;
	}

	virtual bool IsOption_AlwaysAutoAdjustPoints() const
	{
		return _isAlwaysAutoAdjustPoints;
	}

	virtual bool IsOption_CountSubscores() const
	{
		return _isCountSubscores;
	}

	virtual bool IsAwaseteIppon(Score const& s) const
	{
		return IsOption_AwaseteIppon() && s.Wazaari() == GetMaxWazaariCount();
	}

	// second shido will result in yuko beeing added, 3rd will give waza-ari
	virtual bool IsOption_ShidoAddsPoint() const { return false; }
	virtual bool IsOption_ShidoScoreCounts() const { return true; }
	virtual bool IsOption_AwaseteIppon() const { return true; }
	virtual bool IsOption_HasYuko() const { return true; }
	virtual bool IsOption_OpenEndGoldenScore() const { return true; }

	virtual int CompareScore(const Fight& f) const;
	virtual int GetMaxShidoCount() const { return 3; }
	virtual int GetMaxWazaariCount() const { return 2; }
	virtual int GetMaxIpponCount() const { return 1; }
	// Max own offensive points (Ippon/Waza-ari/Yuko) a fighter may gain between
	// one Hajime and the next Mate. INT32_MAX = no cap (all IJF rulesets). The
	// JVP additive system caps a ground sequence at 10 (one Ippon worth) — see
	// Libraries/JVP_Additionssystem.md § 3.5.3. Only the auto-adjust osaekomi
	// escalation honours this (manual scoring stays referee discipline).
	virtual int GetMaxSequencePoints() const { return INT32_MAX; }
	virtual int GetOsaekomiValue(Ipponboard::Score::Point p) const = 0;

	// Numeric value of a score type. Default = the IJF team-list points
	// (see Fight::eScore_*). Additive rulesets (JVP) override this.
	virtual int GetPointValue(Ipponboard::Score::Point p) const;

	// Running additive total (0..20) to show on the scoreboard, or -1 when the
	// ruleset is not additive (then the usual Ippon/Waza-ari/Yuko indicators are
	// shown). Only the JVP additive system returns a value. See [[GetTotalScore]].
	virtual int GetDisplayTotal(const Fight& f, Ipponboard::FighterEnum who) const { return -1; }

	template<typename T>
	bool IsOfType() const { return dynamic_cast<const T*>(this) != nullptr; }

private:
	bool _isAlwaysAutoAdjustPoints { false };
	bool _isCountSubscores { false };
};

class ClassicRules : public AbstractRules
{
public:
	ClassicRules() {}

	static const char* const StaticName;
	virtual const char* Name() const final { return StaticName; }
	virtual bool IsOption_OpenEndGoldenScore() const final { return false; }
	virtual bool IsOption_ShidoAddsPoint() const final { return true; }

	virtual int GetOsaekomiValue(Score::Point p) const final
	{
		switch (p)
		{
		case Score::Point::Ippon: return 25;

		case Score::Point::Wazaari: return 20;

		case Score::Point::Yuko: return 15;

		default: return -1;
		}
	}
};

class Rules2013 : public AbstractRules
{
public:
	Rules2013() {}

	static const char* const StaticName;

	virtual const char* Name() const final { return StaticName; }

	virtual int GetOsaekomiValue(Score::Point p) const final
	{
		switch (p)
		{
		case Score::Point::Ippon: return 20;

		case Score::Point::Wazaari: return 15;

		case Score::Point::Yuko: return 10;

		default: return -1;
		}
	}
};

class Rules2017 : public AbstractRules
{
public:
	Rules2017() {}

	static const char* const StaticName;
	virtual const char* Name() const final { return StaticName; }
	virtual bool IsOption_ShidoScoreCounts() const final { return false; }
	virtual bool IsOption_HasYuko() const final { return false; }
	virtual bool IsOption_AwaseteIppon() const { return false; }
	virtual bool IsAwaseteIppon(Score const&) const final { return false; }
	virtual int GetMaxShidoCount() const final { return 2; }

	virtual int GetOsaekomiValue(Score::Point p) const final
	{
		switch (p)
		{
		case Score::Point::Ippon: return 20;

		case Score::Point::Wazaari: return 10;

		default: return -1;
		}
	}

	virtual int GetMaxWazaariCount() const final { return INT32_MAX; }
};

class Rules2017U15 : public AbstractRules
{
public:
	Rules2017U15() {}

	static const char* const StaticName;
	virtual const char* Name() const final { return StaticName; }
	virtual bool IsOption_ShidoScoreCounts() const final { return false; }
	virtual bool IsOption_HasYuko() const final { return false; }
	virtual bool IsOption_AwaseteIppon() const { return false; }
	virtual bool IsAwaseteIppon(Score const&) const final { return false; }
	virtual int GetMaxShidoCount() const final { return 3; }

	virtual int GetOsaekomiValue(Score::Point p) const final
	{
		switch (p)
		{
		case Score::Point::Ippon: return 20;

		case Score::Point::Wazaari: return 10;

		default: return -1;
		}
	}

	virtual int GetMaxWazaariCount() const final { return INT32_MAX; }
};

class Rules2018 : public AbstractRules
{
public:
	Rules2018() {}

	static const char* const StaticName;
	virtual const char* Name() const final { return StaticName; }
	virtual bool IsOption_ShidoScoreCounts() const final { return false; }
	virtual bool IsOption_HasYuko() const final { return false; }
	virtual bool IsOption_AwaseteIppon() const { return true; }
	virtual int GetMaxShidoCount() const final { return 2; }

	virtual int GetOsaekomiValue(Score::Point p) const final
	{
		switch (p)
		{
		case Score::Point::Ippon: return 20;

		case Score::Point::Wazaari: return 10;

		default: return -1;
		}
	}

	virtual int GetMaxWazaariCount() const final { return 2; }
};

class Rules2025 : public AbstractRules
{
public:
	Rules2025() {}

	static const char* const StaticName;
	virtual const char* Name() const final { return StaticName; }
	virtual bool IsOption_ShidoScoreCounts() const final { return false; }
	virtual bool IsOption_HasYuko() const final { return true; }
	virtual bool IsOption_AwaseteIppon() const { return true; }
	virtual int GetMaxShidoCount() const final { return 2; }

	virtual int GetOsaekomiValue(Score::Point p) const final
	{
		switch (p)
		{
		case Score::Point::Ippon: return 20;

		case Score::Point::Wazaari: return 10;

		case Score::Point::Yuko: return 5;

		default: return -1;
		}
	}

	virtual int GetMaxWazaariCount() const final { return 2; }
};

// Judo-Verband Pfalz "Additionssystem bis 20 Punkte" (WKO 2025 § 3.5) for the
// youth classes U11/U13. Additive, cumulative scoring (Ippon=10, Waza-ari=5,
// Yuko=3, Shido=+2 for the opponent). First to >=20 wins (Sore Made); equal at
// time = Hiki-wake. No Waza-ari-awasete-Ippon, Shido never escalates to
// Hansoku-make (only a direct Hansoku-make decides). See Libraries/JVP_Additionssystem.md.
class RulesPfalzU13 : public AbstractRules
{
public:
	RulesPfalzU13() {}

	static const char* const StaticName;
	virtual const char* Name() const final { return StaticName; }

	virtual bool IsOption_ShidoAddsPoint() const final { return false; }
	virtual bool IsOption_ShidoScoreCounts() const final { return false; }
	virtual bool IsOption_AwaseteIppon() const final { return false; }
	virtual bool IsAwaseteIppon(Score const&) const final { return false; }
	virtual bool IsOption_HasYuko() const final { return true; }
	virtual bool IsOption_OpenEndGoldenScore() const final { return false; }

	// Uncapped accumulation: 2 Ippon (=20), several Waza-ari/Yuko, many Shido.
	virtual int GetMaxShidoCount() const final { return INT32_MAX; }
	virtual int GetMaxWazaariCount() const final { return INT32_MAX; }
	virtual int GetMaxIpponCount() const final { return INT32_MAX; }
	// § 3.5.3: max 10 points (= one Ippon) per Hajime–Mate sequence.
	virtual int GetMaxSequencePoints() const final { return 10; }

	virtual int GetPointValue(Score::Point p) const final
	{
		switch (p)
		{
		case Score::Point::Ippon: return 10;

		case Score::Point::Wazaari: return 5;

		case Score::Point::Yuko: return 3;

		case Score::Point::Shido: return 2; // awarded to the opponent

		default: return 0;
		}
	}

	virtual int GetOsaekomiValue(Score::Point p) const final
	{
		switch (p)
		{
		case Score::Point::Ippon: return 20;

		case Score::Point::Wazaari: return 10;

		case Score::Point::Yuko: return 5;

		default: return -1;
		}
	}

	virtual int CompareScore(const Fight& f) const final;

	// Additive total for a fighter incl. the opponent's Shido (+2 each),
	// clamped to 20 (Sore Made). Reusable by the spectator-screen view (Phase 2).
	int GetTotalScore(const Fight& f, FighterEnum who) const;

	// Scoreboard hook: the additive total (0..20) — the spectator screen shows it.
	virtual int GetDisplayTotal(const Fight& f, FighterEnum who) const final;
};

class RulesFactory
{
public:
	static std::shared_ptr<AbstractRules> Create(QString name)
	{
		if (name == ClassicRules::StaticName)
		{
			return std::make_shared<ClassicRules>();
		}

		if (name == Rules2013::StaticName)
		{
			return std::make_shared<Rules2013>();
		}

		if (name == Rules2017U15::StaticName)
		{
			return std::make_shared<Rules2017U15>();
		}

		if (name == Rules2017::StaticName)
		{
			return std::make_shared<Rules2017>();
		}

		if (name == Rules2018::StaticName)
		{
			return std::make_shared<Rules2018>();
		}

		if (name == Rules2025::StaticName)
		{
			return std::make_shared<Rules2025>();
		}

		if (name == RulesPfalzU13::StaticName)
		{
			return std::make_shared<RulesPfalzU13>();
		}

		// default
		return std::make_shared<Rules2025>();
	}

	static QStringList GetNames()
	{
		auto result = QStringList();

		result.push_back(Rules2025::StaticName);
		result.push_back(Rules2018::StaticName);
		result.push_back(Rules2017::StaticName);
		result.push_back(Rules2017U15::StaticName);
		result.push_back(Rules2013::StaticName);
		result.push_back(ClassicRules::StaticName);
		result.push_back(RulesPfalzU13::StaticName);

		return result;
	}

	static QString GetDefaultName()
	{
		return Rules2025::StaticName;
	}
};
} // namespace
#endif // RULESET_H
