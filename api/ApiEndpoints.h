#ifndef API__APIENDPOINTS_H_
#define API__APIENDPOINTS_H_

#include <QString>

class QTcpSocket;
class QJsonObject;

namespace Ipponboard
{
class Controller;
class FighterManager;

class Fighter;

struct PostFightersResult 
{
    bool success = false;
    QString category;
    QString weightClass;
    QString fighter1Name;
    QString fighter2Name;
    QString callbackUrl;
};

class ApiEndpoints
{
public:
    explicit ApiEndpoints(Controller* pController, FighterManager* pFighterManager);

    PostFightersResult HandlePostFighters(QTcpSocket* pSocket, const QJsonObject& json);

private:
    Fighter ParseFighterFromJson(const QJsonObject& fighterJson);

    Controller* m_pController;
    FighterManager* m_pFighterManager;
};

}

#endif // API__APIENDPOINTS_H_
