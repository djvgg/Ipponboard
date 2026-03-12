#ifndef API__APISERVER_H_
#define API__APISERVER_H_

#include <QTcpServer>
#include <QMap>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <memory>

class QTcpSocket;
class QJsonObject;

namespace Ipponboard
{
class Controller;
class ApiEndpoints;
class FighterManager;
class FightDataDispatcher;

constexpr uint16_t PORT = 8080;

class ApiServer : public QTcpServer
{
    Q_OBJECT

public:
    explicit ApiServer(Controller* pController, FighterManager* pFighterManager, QObject* parent = nullptr);
    ~ApiServer();

    bool StartListening(uint16_t port);
    void StopListening();

    void BroadcastData(const QJsonObject& json);

signals:
    void fightersAdded(const QString& category, const QString& weightClass, const QString& fighter1Name, const QString& fighter2Name);

protected:
    void incomingConnection(qintptr socketDescriptor) override;

private:
    void handleRequest(QTcpSocket* pSocket);
    void SendWebhook(const QJsonObject& json);

    Controller* m_pController;
    FighterManager* m_pFighterManager;
    std::unique_ptr<ApiEndpoints> m_pEndpoints;
    QList<QTcpSocket*> m_clients; 
    QMap<QTcpSocket*, QByteArray> m_buffers;
    QString m_callbackUrl;
    QNetworkAccessManager m_networkManager;
};

}

#endif // API__APISERVER_H_
