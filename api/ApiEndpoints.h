#ifndef API__APIENDPOINTS_H_
#define API__APIENDPOINTS_H_

class QTcpSocket;
class QJsonObject;

namespace Ipponboard
{
class Controller;

class ApiEndpoints
{
public:
    explicit ApiEndpoints(Controller* pController);

    void HandlePostFighter(QTcpSocket* pSocket, const QJsonObject& json);

private:
    Controller* m_pController;
};

}

#endif // API__APIENDPOINTS_H_
