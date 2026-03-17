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
#include <QDebug>


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
            qDebug() << "Dispatcher - Winner detected (waiting for manual SEND):" 
                      << (winner == FighterEnum::First ? "fighter1" : "fighter2");
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

    qDebug() << "ManualDispatch triggered. Current Winner:" 
              << (winner == FighterEnum::First ? "fighter1" : (winner == FighterEnum::Second ? "fighter2" : "none"));

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
    
    // Explicitly use the rules for decision making as requested
    auto pRules = m_pController->GetRules();
    if (!pRules)
    {
        return FighterEnum::Nobody;
    }

    // A winner is only detected if it's a "real" win (Ippon/Hansokumake)
    // or if the time is up (or lead in Golden Score).
    
    // m_pController->GetScore(..., Point::Ippon) already includes rule-based Awasete-Ippon.
    bool s1ippon = m_pController->GetScore(FighterEnum::First, Score::Point::Ippon) > 0;
    bool s2ippon = m_pController->GetScore(FighterEnum::Second, Score::Point::Ippon) > 0;
    bool s1hansoku = m_pController->GetScore(FighterEnum::First, Score::Point::Hansokumake) > 0;
    bool s2hansoku = m_pController->GetScore(FighterEnum::Second, Score::Point::Hansokumake) > 0;

    if (s1ippon || s2hansoku)
    {
        qDebug() << "Dispatcher - Winner:" << (s1ippon ? "Ippon (F1)" : "Hansokumake (F2)");
        return FighterEnum::First;
    }
    
    if (s2ippon || s1hansoku)
    {
        qDebug() << "Dispatcher - Winner:" << (s2ippon ? "Ippon (F2)" : "Hansokumake (F1)");
        return FighterEnum::Second;
    }

    // Live-Timer Check
    int remainingSecs = m_pController->GetSecondsRemaining();

    // Golden Score: The first point (Wazaari/Yuko) wins the fight immediately.
    if (fight.IsGoldenScore())
    {
        if (pRules->CompareScore(fight) != 0)
        {
            FighterEnum winner = m_pController->GetWinner();
            qDebug() << "Dispatcher - Winner: Golden Score Lead (" << (winner == FighterEnum::First ? "F1" : "F2") << ")";
            return winner;
        }
    }
    // Normal fight: Only if time is up (0:00), we check who is leading according to the rules.
    else if (remainingSecs <= 0)
    {
        FighterEnum winner = m_pController->GetWinner();
        if (winner != FighterEnum::Nobody)
        {
            qDebug() << "Dispatcher - Winner: Time elapsed with lead (" << (winner == FighterEnum::First ? "F1" : "F2") << ")";
        }
        return winner;
    }

    return FighterEnum::Nobody;
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
