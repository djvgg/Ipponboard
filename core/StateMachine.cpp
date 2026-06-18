// Copyright 2018 Florian Muecke. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE.txt file.

#include "StateMachine.h"
#include "Controller.h"

#include <QTimer>

using namespace Ipponboard;
using Point = Score::Point;

//---------------------------------------------------------
int IpponboardSM_::own_offensive_points(FighterEnum who) const
//---------------------------------------------------------
{
	auto pRules = m_pCore->GetRules();
	const Score& s = Score_(who);

	return s.Value(Point::Ippon) * pRules->GetPointValue(Point::Ippon)
		   + s.Wazaari() * pRules->GetPointValue(Point::Wazaari)
		   + s.Yuko() * pRules->GetPointValue(Point::Yuko);
}

//---------------------------------------------------------
void IpponboardSM_::add_point(HoldTimeEvent const& evt)
//---------------------------------------------------------
{
	auto pRules = m_pCore->GetRules();

	if (!m_pCore->is_auto_adjust())
	{
		return;
	}

	// Defensive: only a real fighter holds. evt.tori comes from Controller::m_Tori,
	// which is Nobody (-1) by default and across resets; never index m_seqSnapshot
	// (size 2) or score with a non-fighter tori — a Nobody tori is m_seqSnapshot[-1]
	// (UB). The Controller-side guard already blocks this; this is belt-and-braces.
	if (FighterEnum::First != evt.tori && FighterEnum::Second != evt.tori)
	{
		return;
	}

	// JVP § 3.5.3: cap the points gained in one Hajime–Mate sequence. For every
	// IJF ruleset GetMaxSequencePoints() == INT32_MAX, so the escalation below
	// is never suppressed and behaviour is unchanged.
	const int cap = pRules->GetMaxSequencePoints();
	const int snapshot = m_seqSnapshot[static_cast<int>(evt.tori)];
	const int ownNow = own_offensive_points(evt.tori);

	const int vYuko = pRules->GetPointValue(Point::Yuko);
	const int vWaza = pRules->GetPointValue(Point::Wazaari);
	const int vIppon = pRules->GetPointValue(Point::Ippon);

	auto wouldExceedCap = [&](int prospectiveOwnPoints)
	{
		return (prospectiveOwnPoints - snapshot) > cap;
	};

	if (pRules->GetOsaekomiValue(Point::Yuko) == evt.secs)
	{
		if (wouldExceedCap(ownNow + vYuko))
			return;

		Score_(evt.tori).Add(Point::Yuko);
	}
	else if (pRules->GetOsaekomiValue(Point::Wazaari) == evt.secs)
	{
		// escalation replaces the hold's yuko with a waza-ari
		if (wouldExceedCap(ownNow - vYuko + vWaza))
			return;

		Score_(evt.tori).Remove(Point::Yuko);
		Score_(evt.tori).Add(Point::Wazaari);
	}
	else if (pRules->GetOsaekomiValue(Point::Ippon) == evt.secs)
	{
		// escalation replaces the hold's waza-ari with an ippon; suppressed for
		// JVP once the sequence would pass 10 (e.g. after a thrown waza-ari)
		if (wouldExceedCap(ownNow - vWaza + vIppon))
			return;

		Score_(evt.tori).Remove(Point::Wazaari);
		Score_(evt.tori).Add(Point::Ippon);
	}
}

//---------------------------------------------------------
bool IpponboardSM_::can_add_wazaari(Wazaari const& evt)
//---------------------------------------------------------
{
	return m_pCore->GetRules()->IsOption_AwaseteIppon() ||
		   Score_(evt.tori).Wazaari() < m_pCore->GetRules()->GetMaxWazaariCount();
}

//---------------------------------------------------------
bool IpponboardSM_::wazaari_is_match_point(Wazaari const& evt)
//---------------------------------------------------------
{
	return m_pCore->GetRules()->IsOption_AwaseteIppon()
		   && Score_(evt.tori).Wazaari() == m_pCore->GetRules()->GetMaxWazaariCount() - 1;
}

//---------------------------------------------------------
bool IpponboardSM_::has_max_wazaari(RevokeWazaari const& evt)
//---------------------------------------------------------
{
	return m_pCore->GetRules()->IsAwaseteIppon(Score_(evt.tori));
}

//---------------------------------------------------------
bool IpponboardSM_::has_IpponTime(HoldTimeEvent const& evt)
//---------------------------------------------------------
{
	return m_pCore->GetRules()->GetOsaekomiValue(Point::Ippon) == evt.secs;
}

//---------------------------------------------------------
bool IpponboardSM_::has_WazaariTime(HoldTimeEvent const& evt)
//---------------------------------------------------------
{
	return m_pCore->GetRules()->GetOsaekomiValue(Point::Wazaari) == evt.secs;
}

//---------------------------------------------------------
bool IpponboardSM_::has_AwaseteTime(HoldTimeEvent const& evt)
//---------------------------------------------------------
{
	if (m_pCore->GetRules()->IsOption_AwaseteIppon() && 0 != Score_(evt.tori).Wazaari())
	{
		return m_pCore->GetRules()->GetOsaekomiValue(Point::Wazaari) == evt.secs;
	}

	return false;
}

//---------------------------------------------------------
bool IpponboardSM_::has_YukoTime(HoldTimeEvent const& evt)
//---------------------------------------------------------
{
	return m_pCore->GetRules()->GetOsaekomiValue(Point::Yuko) == evt.secs;
}

//---------------------------------------------------------
bool IpponboardSM_::is_sonomama(Osaekomi_Toketa const& /*evt*/)
//---------------------------------------------------------
{
	return m_pCore->is_sonomama();
}

//---------------------------------------------------------
bool IpponboardSM_::shido_is_match_point(Shido const& evt)
//---------------------------------------------------------
{
	if (Score_(evt.tori).Shido() == m_pCore->GetRules()->GetMaxShidoCount())
	{
		return true;
	}

	// Note: new 2013 IJF rule: no points for first three shido
//    if (m_pCore->GetRules()->IsShidosCountAsPoints())
//	{
//		FighterEnum uke = GetUkeFromTori(evt.tori);

//        if (Score_(evt.tori).Shido() == 2 && Score_(uke).Wazaari() == 1)
//		{
//			return true;
//		}
//	}

	return false;
}

//---------------------------------------------------------
bool IpponboardSM_::can_take_shido(Shido const& evt)
//---------------------------------------------------------
{
	return Score_(evt.tori).Shido() <= m_pCore->GetRules()->GetMaxShidoCount();
}

void IpponboardSM_::reset(Reset const& /*evt*/)
{
	m_pCore->reset_fight();
}

void IpponboardSM_::save(Finish const& /*evt*/)
{
	m_pCore->save_fight();
}

void IpponboardSM_::stop_timer(Osaekomi_Toketa const& /*evt*/)
{
	m_pCore->stop_timer(eTimer_Hold);
}

void IpponboardSM_::stop_timer(TimeEndedEvent const& /*evt*/)
{
	m_pCore->stop_timer(eTimer_Main);
}

void IpponboardSM_::stop_timer(Finish const& /*evt*/)
{
	// Finish will be created if current fight should be saved
	// --> stop timers
	m_pCore->stop_timer(eTimer_Hold);
	m_pCore->stop_timer(eTimer_Main);   // will save main time
	m_pCore->save_fight();
}

void IpponboardSM_::stop_timer(Hajime_Mate const& /*evt*/)
{
	m_pCore->stop_timer(ETimer(Hajime_Mate::type));
}

void IpponboardSM_::start_timer(Hajime_Mate const& /*evt*/)
{
	// snapshot offensive points at Hajime so the per-sequence cap (JVP § 3.5.3)
	// measures only what is scored until the next Mate
	m_seqSnapshot[static_cast<int>(FighterEnum::First)] = own_offensive_points(FighterEnum::First);
	m_seqSnapshot[static_cast<int>(FighterEnum::Second)] = own_offensive_points(FighterEnum::Second);

	m_pCore->reset_timer(eTimer_Hold);
	m_pCore->start_timer(eTimer_Main);
}

void IpponboardSM_::start_timer(Osaekomi_Toketa const& /*evt*/)
{
	//m_pCore->reset_timer( eTimer_Hold );
	m_pCore->start_timer(eTimer_Hold);
}

void IpponboardSM_::add_point(PointEvent<ippon_type> const& evt)
{
	Score_(evt.tori).Add(Point::Ippon);
	m_pCore->stop_timer(eTimer_Main);
	m_pCore->stop_timer(eTimer_Hold);
}

void IpponboardSM_::add_point(PointEvent<shido_type> const& evt)
{
	auto pRules = m_pCore->GetRules();

	if (m_pCore->is_auto_adjust())
	{
		FighterEnum uke = GetUkeFromTori(evt.tori);

		auto maxShidoCount = pRules->GetMaxShidoCount();

		if (maxShidoCount == Score_(evt.tori).Shido())
		{
			Score_(uke).Add(Point::Ippon);
		}
		else
		{
			if (pRules->IsOption_ShidoAddsPoint())
			{
				if (maxShidoCount > 2 && 3 == Score_(evt.tori).Shido())
				{
					Score_(uke).Remove(Point::Wazaari);
					Score_(uke).Add(Point::Ippon);
				}
				else if (maxShidoCount > 1 && 2 == Score_(evt.tori).Shido())
				{
					Score_(uke).Remove(Point::Yuko);
					Score_(uke).Add(Point::Wazaari);
				}
				else if (maxShidoCount > 0 && 1 == Score_(evt.tori).Shido())
				{
					Score_(uke).Add(Point::Yuko);
				}
			}
		}
	}

	Score_(evt.tori).Add(Point::Shido);
}

void IpponboardSM_::add_point(PointEvent<revoke_shido_hm_type> const& evt)
{
	FighterEnum uke = GetUkeFromTori(evt.tori);

	if (Score_(evt.tori).Hansokumake())
	{
		// mirror add_point(hansokumake): only the IJF path added the opponent's
		// Ippon, so only revoke it there — otherwise we'd strip a legitimately
		// scored Ippon in the additive system.
		if (m_pCore->GetRules()->IsOption_HansokumakeAwardsIppon())
			Score_(uke).Remove(Point::Ippon);
		Score_(evt.tori).Remove(Point::Hansokumake);
	}
	else
	{
		auto pRules = m_pCore->GetRules();

		if (m_pCore->is_auto_adjust())
		{
			auto maxShidoCount = pRules->GetMaxShidoCount();

			// "fighter is at cap+1 shido" (the hansoku-triggering count). Written as
			// Shido()-1 == cap to avoid cap+1 overflowing when cap is INT32_MAX
			// (JVP/additive, uncapped) — signed overflow is UB. Equivalent for every
			// capped IJF ruleset (Shido() >= 0).
			if (Score_(evt.tori).Shido() - 1 == maxShidoCount)
			{
				Score_(uke).Remove(Point::Ippon);
			}
			// mirror add_point(shido): only undo the shido->point escalation when
			// the ruleset actually awards points per shido. JVP/additive keeps this
			// off (shido is a display-only +2), so a revoke must NOT touch the
			// opponent's real throw-scores. (maxShidoCount+1 == Shido() above mirrors
			// the un-gated hansoku escalation and stays outside this guard.)
			else if (pRules->IsOption_ShidoAddsPoint())
			{
				if (maxShidoCount > 2 && 4 == Score_(evt.tori).Shido())
				{
					Score_(uke).Remove(Point::Ippon);
					Score_(uke).Add(Point::Wazaari);
				}
				else if (maxShidoCount > 1 && 3 == Score_(evt.tori).Shido())
				{
					Score_(uke).Remove(Point::Wazaari);
					Score_(uke).Add(Point::Yuko);
				}
				else if (maxShidoCount > 0 && 2 == Score_(evt.tori).Shido())
				{
					Score_(uke).Remove(Point::Yuko);
				}
			}
		}

		Score_(evt.tori).Remove(Point::Shido);
	}
}

void IpponboardSM_::add_point(PointEvent<hansokumake_type> const& evt)
{
	FighterEnum uke = GetUkeFromTori(evt.tori);
	// IJF: Hansoku-make awards the opponent a full Ippon (= the win). The JVP
	// additive system treats a direct Hansoku-make as a pure DQ (WKO §3.5.2),
	// decided by CompareScore's HM override — it must NOT add 10 points to the
	// opponent's 0..20 total.
	if (m_pCore->GetRules()->IsOption_HansokumakeAwardsIppon())
		Score_(uke).Add(Point::Ippon);
	Score_(evt.tori).Add(Point::Hansokumake);

	m_pCore->stop_timer(eTimer_Main);
	m_pCore->stop_timer(eTimer_Hold);
}

void IpponboardSM_::yoshi(Osaekomi_Toketa const& /*evt*/)
{
	m_pCore->start_timer(eTimer_Main);
	m_pCore->start_timer(eTimer_Hold);
}
