#include "FightDataDispatcher.h"
#include "../core/Controller.h"
#include "../core/Fight.h"
#include "../core/Score.h"
#include "../core/Enums.h"
#include "../core/Rules.h"
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonValue>
#include <iostream>

using namespace Ipponboard;

FightDataDispatcher::FightDataDispatcher(IController* pController)
    : m_pController(pController)
{
}

void FightDataDispatcher::UpdateView()
{
    Controller* pController = dynamic_cast<Controller*>(m_pController);
    if (!pController)
    {
        return;
    }

    int currentFight = pController->GetCurrentFight();
    int currentRound = pController->GetCurrentRound();

    // Get the actual fight object to check its state
    Fight const& fight = pController->GetFight(currentRound, currentFight);

    // We only want to notify when the fight is DECIDED.
    FighterEnum winner = FighterEnum::Nobody;
    auto rules = pController->GetRules();

    bool s1ippon = fight.GetScore1().Ippon() > 0 || (rules && rules->IsAwaseteIppon(fight.GetScore1()));
    bool s2ippon = fight.GetScore2().Ippon() > 0 || (rules && rules->IsAwaseteIppon(fight.GetScore2()));
    bool s1hansoku = fight.GetScore1().Hansokumake();
    bool s2hansoku = fight.GetScore2().Hansokumake();

    if (s1ippon)
    {
        winner = FighterEnum::First;
    }
    else if (s2ippon)
    {
        winner = FighterEnum::Second;
    }
    else if (s1hansoku)
    {
        winner = FighterEnum::Second;
    }
    else if (s2hansoku)
    {
        winner = FighterEnum::First;
    }
    else if (fight.is_saved)
    {
        winner = pController->GetWinner();
    }

    // Check if we already reported a winner for this fight
    if (m_lastFightIndex == currentFight && 
        m_lastRoundIndex == currentRound && 
        m_lastWinner == winner)
    {
        return;
    }

    // Only send if there is a winner
    if (winner == FighterEnum::Nobody)
    {
        // If it's a new fight or winner was cleared, update our state
        if (m_lastFightIndex != currentFight || m_lastRoundIndex != currentRound || m_lastWinner != FighterEnum::Nobody)
        {
            m_lastFightIndex = currentFight;
            m_lastRoundIndex = currentRound;
            m_lastWinner = FighterEnum::Nobody;
            std::cout << "DEBUG: Dispatcher::UpdateView - No winner (resetting last reported winner)" << std::endl;
        }
        return;
    }

    // Update state
    m_lastFightIndex = currentFight;
    m_lastRoundIndex = currentRound;
    m_lastWinner = winner;

    std::cout << "DEBUG: Dispatcher::UpdateView - WINNER DECIDED: " 
              << (winner == FighterEnum::First ? "fighter1" : "fighter2") 
              << " (S1_Ippon:" << (s1ippon?"Y":"N") 
              << ", S2_Ippon:" << (s2ippon?"Y":"N")
              << ", S1_HM:" << (s1hansoku?"Y":"N")
              << ", S2_HM:" << (s2hansoku?"Y":"N")
              << ", Saved:" << (fight.is_saved?"Y":"N") << ")" << std::endl;

    QJsonObject data;
    
    // Fighter information
    QJsonObject fighter1;
    fighter1["name"] = QJsonValue(pController->GetFighterName(FighterEnum::First));
    
    QJsonObject fighter2;
    fighter2["name"] = QJsonValue(pController->GetFighterName(FighterEnum::Second));
    
    data["fighter1"] = QJsonValue(fighter1);
    data["fighter2"] = QJsonValue(fighter2);
    data["time"] = QJsonValue(pController->GetTimeText(eTimer_Main));
    data["winner"] = QJsonValue(winner == FighterEnum::First ? "fighter1" : "fighter2");

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
