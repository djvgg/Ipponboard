#include "FightDataDispatcher.h"
#include "../core/Controller.h"
#include "../core/Fight.h"
#include "../core/Score.h"
#include "../core/Enums.h"
#include "../core/Rules.h"
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonValue>
#include <QTime>
#include <iostream>


using namespace Ipponboard;

FightDataDispatcher::FightDataDispatcher(IController* pController)
    : m_pController(pController)
{
}

void FightDataDispatcher::UpdateView()
{
    Controller* pController = dynamic_cast<Controller*>(m_pController);
    if (!pController) return;

    // We still track the state internally, but we don't emit anymore.
    // This allows us to log "Winner Decided" events without spamming the API.
    int currentFight = pController->GetCurrentFight();
    int currentRound = pController->GetCurrentRound();
    Fight const& fight = pController->GetFight(currentRound, currentFight);
    
    // Use the controller's GetWinner() as the primary source of truth, 
    // especially for Golden Score and saved fights.
    FighterEnum winner = pController->GetWinner();
    auto rules = pController->GetRules();

    // Fallback: Check for Ippon or Hansokumake specifically if not caught above
    if (winner == FighterEnum::Nobody)
    {
        bool s1ippon = fight.GetScore1().Ippon() > 0 || (rules && rules->IsAwaseteIppon(fight.GetScore1()));
        bool s2ippon = fight.GetScore2().Ippon() > 0 || (rules && rules->IsAwaseteIppon(fight.GetScore2()));
        bool s1hansoku = fight.GetScore1().Hansokumake();
        bool s2hansoku = fight.GetScore2().Hansokumake();

        if (s1ippon) winner = FighterEnum::First;
        else if (s2ippon) winner = FighterEnum::Second;
        else if (s1hansoku) winner = FighterEnum::Second;
        else if (s2hansoku) winner = FighterEnum::First;
    }

    if (m_lastFightIndex != currentFight || m_lastRoundIndex != currentRound || m_lastWinner != winner)
    {
        m_lastFightIndex = currentFight;
        m_lastRoundIndex = currentRound;
        m_lastWinner = winner;
        
        if (winner != FighterEnum::Nobody)
        {
            std::cout << "DEBUG: Dispatcher - Winner detected (waiting for manual SEND): " 
                      << (winner == FighterEnum::First ? "fighter1" : "fighter2") << std::endl;
        }
    }
}

void FightDataDispatcher::ManualDispatch()
{
    Controller* pController = dynamic_cast<Controller*>(m_pController);
    if (!pController) return;

    int currentFight = pController->GetCurrentFight();
    int currentRound = pController->GetCurrentRound();
    Fight const& fight = pController->GetFight(currentRound, currentFight);
    
    // Use the controller's GetWinner() as the primary source of truth
    FighterEnum winner = pController->GetWinner();
    auto rules = pController->GetRules();

    if (winner == FighterEnum::Nobody)
    {
        bool s1ippon = fight.GetScore1().Ippon() > 0 || (rules && rules->IsAwaseteIppon(fight.GetScore1()));
        bool s2ippon = fight.GetScore2().Ippon() > 0 || (rules && rules->IsAwaseteIppon(fight.GetScore2()));
        bool s1hansoku = fight.GetScore1().Hansokumake();
        bool s2hansoku = fight.GetScore2().Hansokumake();

        if (s1ippon) winner = FighterEnum::First;
        else if (s2ippon) winner = FighterEnum::Second;
        else if (s1hansoku) winner = FighterEnum::Second;
        else if (s2hansoku) winner = FighterEnum::First;
    }

    std::cout << "DEBUG: ManualDispatch triggered. Current Winner: " 
              << (winner == FighterEnum::First ? "fighter1" : (winner == FighterEnum::Second ? "fighter2" : "none")) 
              << std::endl;

    QJsonObject data;
    
    // Fighter information
    QJsonObject fighter1;
    fighter1["name"] = QJsonValue(pController->GetFighterName(FighterEnum::First));
    
    QJsonObject fighter2;
    fighter2["name"] = QJsonValue(pController->GetFighterName(FighterEnum::Second));
    
    data["fighter1"] = QJsonValue(fighter1);
    data["fighter2"] = QJsonValue(fighter2);
    // Calculate total elapsed time (including Golden Score)
    QString mainTimeStr = pController->GetTimeText(eTimer_Main);
    QTime mainTime = QTime::fromString(mainTimeStr, "m:ss");
    int mainTimeSecs = mainTime.isValid() ? QTime(0, 0).secsTo(mainTime) : 0;
    
    // Get the base round time from the controller (more reliable than the fight object)
    int baseTimeSecs = pController->GetFightDuration(fight.weight);
    if (baseTimeSecs <= 0)
    {
        // Fallback: try to get it from the displayed round time
        QTime baseTime = QTime::fromString(pController->GetFightTimeString(), "m:ss");
        if (baseTime.isValid())
            baseTimeSecs = QTime(0, 0).secsTo(baseTime);
        else
            baseTimeSecs = 180; // Last resort: 3 minutes
    }
    
    int totalSecs = 0;
    if (pController->IsGoldenScore())
    {
        // In Golden Score, mainTime counts UP from 0:00
        totalSecs = baseTimeSecs + mainTimeSecs;
    }
    else
    {
        // In regular time, mainTime counts DOWN to 0:00
        totalSecs = baseTimeSecs - mainTimeSecs;
        if (totalSecs < 0) totalSecs = 0; 
    }
    
    QString totalTimeStr = QString("%1:%2").arg(totalSecs / 60).arg(abs(totalSecs % 60), 2, 10, QChar('0'));
    data["time"] = QJsonValue(totalTimeStr);
    data["winner"] = QJsonValue(winner == FighterEnum::First ? "fighter1" : (winner == FighterEnum::Second ? "fighter2" : ""));

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
