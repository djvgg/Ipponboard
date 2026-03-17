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
    
    if (!pController)
    {
        return;
    } 

    int currentFight = pController->GetCurrentFight();
    int currentRound = pController->GetCurrentRound();
    FighterEnum winner = CalculateWinner();

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
    if (!pController)
    {
        return;
    }

    FighterEnum winner = CalculateWinner();
    QString totalTimeStr = CalculateTotalTime();

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
    data["time"] = QJsonValue(totalTimeStr);
    data["winner"] = QJsonValue(winner == FighterEnum::First ? "fighter1" : (winner == FighterEnum::Second ? "fighter2" : ""));

    emit dataUpdated(data);
}

FighterEnum FightDataDispatcher::CalculateWinner() const
{
    Controller* pController = dynamic_cast<Controller*>(m_pController);
    if (!pController)
    {
        return FighterEnum::Nobody;
    }

    int currentFight = pController->GetCurrentFight();
    int currentRound = pController->GetCurrentRound();
    Fight const& fight = pController->GetFight(currentRound, currentFight);
    
    FighterEnum winner = pController->GetWinner();
    auto rules = pController->GetRules();

    if (winner == FighterEnum::Nobody)
    {
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
    }

    return winner;
}

QString FightDataDispatcher::CalculateTotalTime() const
{
    Controller* pController = dynamic_cast<Controller*>(m_pController);
    if (!pController)
    {
        return "0:00";
    }

    int currentFight = pController->GetCurrentFight();
    int currentRound = pController->GetCurrentRound();
    Fight const& fight = pController->GetFight(currentRound, currentFight);

    QString mainTimeStr = pController->GetTimeText(eTimer_Main);
    QTime mainTime = QTime::fromString(mainTimeStr, "m:ss");
    int mainTimeSecs = mainTime.isValid() ? QTime(0, 0).secsTo(mainTime) : 0;
    
    // Get the base round time from the controller (more reliable than the fight object)
    int baseTimeSecs = pController->GetFightDuration(fight.weight);
    QTime baseTime = QTime::fromString(pController->GetFightTimeString(), "m:ss");
    if (baseTimeSecs <= 0 && baseTime.isValid())
    {
        baseTimeSecs = QTime(0, 0).secsTo(baseTime);
    }
    
    int totalSecs = 0;
    if (pController->IsGoldenScore())
    {
        totalSecs = baseTimeSecs + mainTimeSecs;
    }
    else
    {
        totalSecs = baseTimeSecs - mainTimeSecs;
        if (totalSecs < 0) totalSecs = 0; 
    }
    
    return QString("%1:%2").arg(totalSecs / 60).arg(abs(totalSecs % 60), 2, 10, QChar('0'));
}

void FightDataDispatcher::Reset()
{
    UpdateView();
}

void FightDataDispatcher::SetShowInfoHeader(bool /*show*/)
{
    // Not relevant for API dispatcher
}
