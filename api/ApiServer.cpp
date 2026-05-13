#include "ApiServer.h"
#include "ApiEndpoints.h"
#include "HttpResponse.h"
#include <QTcpSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QUrl>
#include <QDebug>
#include <cstdint>

using namespace Ipponboard;

ApiServer::ApiServer(Controller* pController, FighterManager* pFighterManager, QObject* parent)
    : QTcpServer(parent)
    , m_pController(pController)
    , m_pFighterManager(pFighterManager)
    , m_pEndpoints(std::make_unique<ApiEndpoints>(pController, pFighterManager))
{
}

ApiServer::~ApiServer()
{
    StopListening();
}

bool ApiServer::StartListening(std::uint16_t port)
{
    if (!listen(QHostAddress::Any, port))
    {
        return false;
    }
    return true;
}

void ApiServer::StopListening()
{
    if (isListening())
    {
        close();
    }
}

void ApiServer::incomingConnection(qintptr socketDescriptor)
{
    QTcpSocket* pSocket = new QTcpSocket(this);
    if (pSocket->setSocketDescriptor(socketDescriptor))
    {

        connect(pSocket, &QTcpSocket::disconnected, this, [this, pSocket]()
        {
            m_clients.removeAll(pSocket);
            m_buffers.remove(pSocket);
            pSocket->deleteLater();
        });

        m_clients.append(pSocket);
        qInfo() << "New client connected:" << pSocket->peerAddress().toString() << "(Total:" << m_clients.size() << ")";


        connect(pSocket, &QTcpSocket::readyRead, this, [this, pSocket]()
        {
            handleRequest(pSocket);
        });
    }
    else
    {
        delete pSocket;
    }
}

void ApiServer::handleRequest(QTcpSocket* pSocket)
{
    m_buffers[pSocket].append(pSocket->readAll());
    processBuffer(pSocket);
}

void ApiServer::processBuffer(QTcpSocket* pSocket)
{
    QByteArray& buffer = m_buffers[pSocket];
    QString method, path, body;

    while (parseNextHttpRequest(buffer, method, path, body))
    {
        routeRequest(pSocket, method, path, body);
        if (buffer.isEmpty()) break;
    }
}

bool ApiServer::parseNextHttpRequest(QByteArray& buffer, QString& method, QString& path, QString& body)
{
    int headerEnd = buffer.indexOf("\r\n\r\n");
    if (headerEnd == -1)
    {
        return false; // Header noch nicht vollständig
    }

    QString headers = QString::fromUtf8(buffer.left(headerEnd));
    
    // Content-Length finden
    int contentLength = 0;
    int clPos = headers.indexOf("Content-Length:", 0, Qt::CaseInsensitive);
    if (clPos != -1)
    {
        int lineEnd = headers.indexOf("\r\n", clPos);
        QString clLine = headers.mid(clPos, lineEnd - clPos);
        contentLength = clLine.section(':', 1).trimmed().toInt();
    }

    int totalSize = headerEnd + 4 + contentLength;
    if (buffer.size() < totalSize)
    {
        return false; // Body noch nicht vollständig
    }

    // Wir haben einen kompletten Request!
    QByteArray fullRequestData = buffer.left(totalSize);
    buffer.remove(0, totalSize);

    QString request = QString::fromUtf8(fullRequestData);
    QString startLine = request.section("\r\n", 0, 0);
    method = startLine.section(' ', 0, 0);
    path = startLine.section(' ', 1, 1);
    body = request.mid(headerEnd + 4);

    return true;
}

void ApiServer::routeRequest(QTcpSocket* pSocket, const QString& method, const QString& path, const QString& body)
{
    qInfo() << "Request:" << method << path << "(Body:" << body.length() << "bytes)";

    if (method == "POST" && path == "/fighters")
    {
        QJsonDocument doc = QJsonDocument::fromJson(body.toUtf8());
        if (doc.isNull() || !doc.isObject())
        {
            HttpResponse::Send(pSocket, HttpResponse::StatusCode::BadRequest, "Bad Request: Invalid JSON");
        }
        else
        {
            qDebug() << "Received JSON:" << doc.toJson(QJsonDocument::Compact);
            auto result = m_pEndpoints->HandlePostFighters(pSocket, doc.object());
            if (result.success)
            {
                // We ignore the callback from JSON and use the sender's IP instead
                QString senderIp = pSocket->peerAddress().toString();
                if (senderIp.startsWith("::ffff:")) senderIp.remove("::ffff:");

                // Bracket IPv6 host so the URL parses correctly (e.g. http://[::1]:5001/...)
                QString hostForUrl = senderIp.contains(':') ? QString("[%1]").arg(senderIp) : senderIp;
                m_callbackUrl = QString("http://%1:%2/api/ippon-score").arg(hostForUrl).arg(m_websitePort);

                qInfo() << "Callback URL set automatically to:" << m_callbackUrl << "(Using WebsitePort:" << m_websitePort << ")";
                emit fightersAdded(result.category, result.weightClass, result.fighter1Name, result.fighter2Name);
            }
        }
    }
    else
    {
        HttpResponse::Send(pSocket, HttpResponse::StatusCode::NotFound, "Not Found");
    }
}

void ApiServer::BroadcastData(const QJsonObject& json)
{
    QJsonDocument doc(json);
    QByteArray jsonData = doc.toJson(QJsonDocument::Compact);
    qDebug() << "Broadcasting JSON:" << jsonData;

    // Always send webhook if configured
    SendWebhook(json);

    if (m_clients.isEmpty())
    {
        return;
    }

    jsonData.append("\n"); // Newline for easier parsing by simple clients

    for (QTcpSocket* pClient : m_clients)
    {
        if (pClient->state() == QAbstractSocket::ConnectedState)
        {
            QString ip = pClient->peerAddress().toString();
            if (ip.startsWith("::ffff:")) ip.remove("::ffff:");
            
            qDebug() << "Sending to client" << ip;
            pClient->write(jsonData);
        }
    }
}

void ApiServer::SendWebhook(const QJsonObject& json)
{
    if (m_callbackUrl.isEmpty())
    {
        return;
    }

    QString finalUrl = m_callbackUrl;
    if (!finalUrl.startsWith("http://") && !finalUrl.startsWith("https://"))
    {
        finalUrl = "http://" + finalUrl;
    }
    
    // Fallback: If no path is provided, append /api/ippon-score
    QUrl url(finalUrl);
    if (url.path().isEmpty() || url.path() == "/")
    {
        url.setPath("/api/ippon-score");
    }

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonDocument doc(json);
    QByteArray data = doc.toJson(QJsonDocument::Compact);

    qInfo() << "Sending Webhook to" << url.toString();
    
    QNetworkReply* pReply = m_networkManager.post(request, data);
    
    // Auto-delete reply when done and log result
    connect(pReply, &QNetworkReply::finished, [pReply]() {
        int statusCode = pReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (pReply->error() == QNetworkReply::NoError) {
            qInfo() << "Webhook delivered successfully! Status:" << statusCode;
        } else {
            qCritical() << "Webhook FAILED:" << pReply->errorString() 
                       << "(Status:" << statusCode << ")";
        }
        pReply->deleteLater();
    });
}
