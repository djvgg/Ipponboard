#include "ApiServer.h"
#include <QTcpSocket>

using namespace Ipponboard;

ApiServer::ApiServer(Controller* pController, QObject* parent): QTcpServer(parent), m_pController(pController)
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
        // Wir schicken eine minimale HTTP-Antwort
        pSocket->write("HTTP/1.1 200 OK\r\n");
        pSocket->write("Content-Type: text/plain\r\n");
        pSocket->write("\r\n");
        pSocket->write("Ipponboard API is alive!");
        
        // Wir warten kurz, bis die Daten gesendet wurden, dann erst auflegen
        pSocket->disconnectFromHost();
    }
    else
    {
        delete pSocket;
    }
}
