#include "ApiServer.h"
#include "ApiEndpoints.h"
#include "HttpResponse.h"
#include <QTcpSocket>
#include <QJsonDocument>
#include <QJsonObject>
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
        connect(pSocket, &QTcpSocket::disconnected, pSocket, &QTcpSocket::deleteLater);

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
    QByteArray requestData = pSocket->readAll();
    QString request = QString::fromUtf8(requestData);

    // HTTP-Startzeile parsen (z.B. "POST /fighters HTTP/1.1")
    QString startLine = request.section("\r\n", 0, 0);
    QString method = startLine.section(' ', 0, 0);   // "POST"
    QString path = startLine.section(' ', 1, 1);      // "/fighters"

    std::cout << "Request: " << method.toStdString() << " " << path.toStdString() << std::endl;

    // JSON-Body extrahieren (alles nach der Leerzeile \r\n\r\n)
    int bodyStart = request.indexOf("\r\n\r\n");
    if (bodyStart == -1) // Wenn keine Zeichenkette gefunden wird, wird -1 zurückgegeben
    {
        HttpResponse::Send(pSocket, HttpResponse::StatusCode::BadRequest, "Bad Request: No body found");
        return;
    }

    int startPosition = bodyStart + 4;

    QString body = request.mid(startPosition);

    // Routing: Welcher Endpunkt wurde aufgerufen?
    if (method == "POST" && path == "/fighters")
    {
        QJsonDocument doc = QJsonDocument::fromJson(body.toUtf8());
        if (doc.isNull() || !doc.isObject())
        {
            HttpResponse::Send(pSocket, HttpResponse::StatusCode::BadRequest, "Bad Request: Invalid JSON");
            return;
        }
        auto result = m_pEndpoints->HandlePostFighters(pSocket, doc.object());
        if (result.success)
        {
            emit fightersAdded(result.category, result.weightClass, result.fighter1Name, result.fighter2Name);
        }
    }
    else
    {
        HttpResponse::Send(pSocket, HttpResponse::StatusCode::NotFound, "Not Found");
    }
}
