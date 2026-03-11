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
class FighterManager;

constexpr uint16_t PORT = 8080;

class ApiServer : public QTcpServer
{
    Q_OBJECT

public:
    explicit ApiServer(Controller* pController, FighterManager* pFighterManager, QObject* parent = nullptr);
    ~ApiServer();

    bool StartListening(uint16_t port);
    void StopListening();

signals:
    void fightersAdded(const QString& category, const QString& weightClass, const QString& fighter1Name, const QString& fighter2Name);

protected:
    void incomingConnection(qintptr socketDescriptor) override;

private:
    void handleRequest(QTcpSocket* pSocket);

    Controller* m_pController;
    FighterManager* m_pFighterManager;
    std::unique_ptr<ApiEndpoints> m_pEndpoints;
};

}

#endif // API__APISERVER_H_
