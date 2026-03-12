#include "FightDataDispatcher.h"
#include "../core/iController.h"
#include "../core/Score.h"
#include "../core/Enums.h"
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonValue>
#include "../core/Rules.h"
#include <iostream>

using namespace Ipponboard;

FightDataDispatcher::FightDataDispatcher(IController* pController)
    : m_pController(pController)
{
}

void FightDataDispatcher::UpdateView()
{
    if (!m_pController)
    {
        return;
    }

    std::cout << "DEBUG: Dispatcher::UpdateView triggered" << std::endl;
    QJsonObject data;
    
    // Fighter information
    QJsonObject fighter1;
    fighter1["name"] = QJsonValue(m_pController->GetFighterName(FighterEnum::First));
    fighter1["ippon"] = QJsonValue(m_pController->GetScore(FighterEnum::First, Score::Point::Ippon));
    fighter1["wazaari"] = QJsonValue(m_pController->GetScore(FighterEnum::First, Score::Point::Wazaari));
    fighter1["yuko"] = QJsonValue(m_pController->GetScore(FighterEnum::First, Score::Point::Yuko));
    fighter1["shido"] = QJsonValue(m_pController->GetScore(FighterEnum::First, Score::Point::Shido));
    
    QJsonObject fighter2;
    fighter2["name"] = QJsonValue(m_pController->GetFighterName(FighterEnum::Second));
    fighter2["ippon"] = QJsonValue(m_pController->GetScore(FighterEnum::Second, Score::Point::Ippon));
    fighter2["wazaari"] = QJsonValue(m_pController->GetScore(FighterEnum::Second, Score::Point::Wazaari));
    fighter2["yuko"] = QJsonValue(m_pController->GetScore(FighterEnum::Second, Score::Point::Yuko));
    fighter2["shido"] = QJsonValue(m_pController->GetScore(FighterEnum::Second, Score::Point::Shido));
    
    data["fighter1"] = QJsonValue(fighter1);
    data["fighter2"] = QJsonValue(fighter2);
    
    // Fight metadata
    data["time"] = QJsonValue(m_pController->GetTimeText(eTimer_Main));
    data["isGoldenScore"] = QJsonValue(m_pController->IsGoldenScore());
    
    // Winner detection logic
    QString winnerStr = "none";
    auto rules = m_pController->GetRules();
    
    bool firstHasIppon = m_pController->GetScore(FighterEnum::First, Score::Point::Ippon) > 0;
    bool secondHasIppon = m_pController->GetScore(FighterEnum::Second, Score::Point::Ippon) > 0;
    bool firstHasHansokumake = m_pController->GetScore(FighterEnum::First, Score::Point::Hansokumake) > 0;
    bool secondHasHansokumake = m_pController->GetScore(FighterEnum::Second, Score::Point::Hansokumake) > 0;
    
    int wazaari1 = m_pController->GetScore(FighterEnum::First, Score::Point::Wazaari);
    int wazaari2 = m_pController->GetScore(FighterEnum::Second, Score::Point::Wazaari);
    
    bool firstHasAwaseteIppon = rules->IsOption_AwaseteIppon() && wazaari1 >= rules->GetMaxWazaariCount();
    bool secondHasAwaseteIppon = rules->IsOption_AwaseteIppon() && wazaari2 >= rules->GetMaxWazaariCount();

    if (firstHasIppon || firstHasAwaseteIppon || secondHasHansokumake)
    {
        winnerStr = "fighter1";
    }
    else if (secondHasIppon || secondHasAwaseteIppon || firstHasHansokumake)
    {
        winnerStr = "fighter2";
    }
    else if (m_pController->GetCurrentState() == eState_TimerStopped)
    {
        // If timer is stopped and we have a lead (e.g. end of fight), consider them winner
        FighterEnum lead = m_pController->GetLead();
        if (lead == FighterEnum::First) winnerStr = "fighter1";
        else if (lead == FighterEnum::Second) winnerStr = "fighter2";
    }
    
    data["winner"] = QJsonValue(winnerStr);

    emit dataUpdated(data);
}

void FightDataDispatcher::Reset()
{
    UpdateView();
}

void FightDataDispatcher::SetShowInfoHeader(bool /*show*/)
{
    // Not relevant for API dispatcher
}
