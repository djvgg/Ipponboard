#include "ApiServer.h"
#include "ApiEndpoints.h"
#include "HttpResponse.h"
#include <QTcpSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QUrl>
#include <iostream>

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

bool ApiServer::StartListening(uint16_t port)
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
        // Sobald die Leitung zu ist -> Objekt automatisch löschen
        connect(pSocket, &QTcpSocket::disconnected, this, [this, pSocket]()
        {
            m_clients.removeAll(pSocket);
            m_buffers.remove(pSocket);
            pSocket->deleteLater();
        });

        m_clients.append(pSocket);
        std::cout << "New client connected: " << pSocket->peerAddress().toString().toStdString() << " (Total: " << m_clients.size() << ")" << std::endl;

        // Warten bis der Client seine Daten geschickt hat
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
    QByteArray& buffer = m_buffers[pSocket];

    // Solange wir vollständige Requests im Puffer haben, diese abarbeiten
    while (true)
    {
        int headerEnd = buffer.indexOf("\r\n\r\n");
        if (headerEnd == -1) break; // Header noch nicht vollständig

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
        if (buffer.size() < totalSize) break; // Body noch nicht vollständig

        // Wir haben einen kompletten Request!
        QByteArray fullRequestData = buffer.left(totalSize);
        buffer.remove(0, totalSize);

        QString request = QString::fromUtf8(fullRequestData);
        QString startLine = request.section("\r\n", 0, 0);
        QString method = startLine.section(' ', 0, 0);
        QString path = startLine.section(' ', 1, 1);
        QString body = request.mid(headerEnd + 4);

        std::cout << "Request: " << method.toStdString() << " " << path.toStdString() << " (Body: " << body.length() << " bytes)" << std::endl;

        if (method == "POST" && path == "/fighters")
        {
            QJsonDocument doc = QJsonDocument::fromJson(body.toUtf8());
            if (doc.isNull() || !doc.isObject())
            {
                HttpResponse::Send(pSocket, HttpResponse::StatusCode::BadRequest, "Bad Request: Invalid JSON");
            }
            else
            {
                std::cout << "DEBUG: Received JSON: " << doc.toJson(QJsonDocument::Compact).toStdString() << std::endl;
                auto result = m_pEndpoints->HandlePostFighters(pSocket, doc.object());
                if (result.success)
                {
                    m_callbackUrl = result.callbackUrl;
                    std::cout << "DEBUG: Callback URL set to: " << m_callbackUrl.toStdString() << std::endl;
                    emit fightersAdded(result.category, result.weightClass, result.fighter1Name, result.fighter2Name);
                }
            }
        }
        else
        {
            HttpResponse::Send(pSocket, HttpResponse::StatusCode::NotFound, "Not Found");
        }

        if (buffer.isEmpty()) break;
    }
}

void ApiServer::BroadcastData(const QJsonObject& json)
{
    QJsonDocument doc(json);
    QByteArray jsonData = doc.toJson(QJsonDocument::Compact);
    std::cout << "DEBUG: Broadcasting JSON: " << jsonData.toStdString() << std::endl;

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
            
            std::cout << "DEBUG: Sending to client " << ip.toStdString() << std::endl;
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

    std::cout << "DEBUG: Sending Webhook to " << url.toString().toStdString() << std::endl;
    
    QNetworkReply* pReply = m_networkManager.post(request, data);
    
    // Auto-delete reply when done and log result
    connect(pReply, &QNetworkReply::finished, [pReply]() {
        if (pReply->error() == QNetworkReply::NoError) {
            std::cout << "DEBUG: Webhook delivered successfully! (HTTP 200/201)" << std::endl << std::flush;
        } else {
            std::cout << "DEBUG: Webhook FAILED: " << pReply->errorString().toStdString() 
                      << " (Code: " << pReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() << ")" 
                      << std::endl << std::flush;
        }
        pReply->deleteLater();
    });
}
