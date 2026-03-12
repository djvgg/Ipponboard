#ifndef API__FIGHTDATADISPATCHER_H_
#define API__FIGHTDATADISPATCHER_H_

#include "../core/iView.h"
#include <QObject>
#include <QJsonObject>

namespace Ipponboard
{
class IController;

class FightDataDispatcher : public QObject, public IView // IView wird immer aufgerufen wenn sich was im Kampf ändert
{
    Q_OBJECT
public:
    explicit FightDataDispatcher(IController* pController);

    // IView interface
    void UpdateView() override;
    void Reset() override;
    void SetShowInfoHeader(bool show) override;

signals:
    void dataUpdated(const QJsonObject& json); // Wenn Daten aktualisiert werden, dann wird BroadCast ausgelöst von ApiServer

private:
    IController* m_pController;
};

}

#endif // API__FIGHTDATADISPATCHER_H_
