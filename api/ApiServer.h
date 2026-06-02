#ifndef API__APISERVER_H_
#define API__APISERVER_H_

#include <QTcpServer>
#include <QMap>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <memory>
#include <cstdint>

class QTcpSocket;
class QJsonObject;

namespace Ipponboard
{
class Controller;
class ApiEndpoints;
class FighterManager;
class FightDataDispatcher;

constexpr std::uint16_t DEFAULT_IPPONBOARD_PORT = 8080;
constexpr std::uint16_t DEFAULT_WEBSITE_PORT = 5001;

class ApiServer : public QTcpServer
{
    Q_OBJECT

public:
    explicit ApiServer(Controller* pController, FighterManager* pFighterManager, QObject* parent = nullptr);
    ~ApiServer();

    void SetWebsitePort(std::uint16_t port)
    {
        m_websitePort = port;
    }
    bool StartListening(std::uint16_t port = DEFAULT_IPPONBOARD_PORT);
    void StopListening();

    void BroadcastData(const QJsonObject& json);

signals:
    void fightersAdded(const QString& category, const QString& weightClass, const QString& fighter1Name, const QString& fighter2Name, const QString& pool);

protected:
    void incomingConnection(qintptr socketDescriptor) override;

private:
    void handleRequest(QTcpSocket* pSocket);
    void processBuffer(QTcpSocket* pSocket);
    bool parseNextHttpRequest(QByteArray& buffer, QString& method, QString& path, QString& body);
    void routeRequest(QTcpSocket* pSocket, const QString& method, const QString& path, const QString& body);
    void SendWebhook(const QJsonObject& json);

    Controller* m_pController;
    FighterManager* m_pFighterManager;
    std::unique_ptr<ApiEndpoints> m_pEndpoints;
    QList<QTcpSocket*> m_clients; 
    QMap<QTcpSocket*, QByteArray> m_buffers;
    QString m_callbackUrl;
    QNetworkAccessManager m_networkManager;
    std::uint16_t m_websitePort = DEFAULT_WEBSITE_PORT;
};

}

#endif // API__APISERVER_H_
