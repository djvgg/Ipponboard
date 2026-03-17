#ifndef API__FIGHTDATADISPATCHER_H_
#define API__FIGHTDATADISPATCHER_H_

#include "../core/iView.h"
#include <QObject>
#include <QString>
#include <QJsonObject>

namespace Ipponboard
{
class IController;

class FightDataDispatcher : public QObject, public IView
{
    Q_OBJECT
public:
    explicit FightDataDispatcher(IController* pController);

    // IView interface
    void UpdateView() override;
    void Reset() override;
    void SetShowInfoHeader(bool show) override;
    
    // Manual trigger for API broadcast
    void ManualDispatch();

signals:
    void dataUpdated(const QJsonObject& json);

private:
    QString CalculateTotalTime() const;
    Ipponboard::FighterEnum CalculateWinner() const;

    IController* m_pController;
    int m_lastFightIndex = -1;
    int m_lastRoundIndex = -1;
    FighterEnum m_lastWinner = FighterEnum::Nobody;
};

}

#endif // API__FIGHTDATADISPATCHER_H_
