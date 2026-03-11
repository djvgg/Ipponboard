#ifndef API__APISERVER_H_
#define API__APISERVER_H_

#include <QTcpServer>
#include <memory>

class QTcpSocket;
class QJsonObject;

namespace Ipponboard
{
class Controller;
class ApiEndpoints;

constexpr uint16_t PORT = 8080;

class ApiServer : public QTcpServer
{
    Q_OBJECT

public:
    explicit ApiServer(Controller* pController, QObject* parent = nullptr);
    ~ApiServer();

    bool StartListening(uint16_t port);
    void StopListening();

protected:
    void incomingConnection(qintptr socketDescriptor) override;

private:
    void handleRequest(QTcpSocket* pSocket);

    Controller* m_pController;
    std::unique_ptr<ApiEndpoints> m_pEndpoints;
};

}

#endif // API__APISERVER_H_
