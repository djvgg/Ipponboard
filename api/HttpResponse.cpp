#include "HttpResponse.h"
#include <QTcpSocket>

using namespace Ipponboard;

void HttpResponse::Send(QTcpSocket* pSocket, StatusCode code, const QString& body)
{
    int statusInt = static_cast<int>(code);
    QString statusText = GetStatusText(code);

    QByteArray bodyUtf8 = body.toUtf8();
    QString response = QString("HTTP/1.1 %1 %2\r\n"
                               "Content-Type: text/plain\r\n"
                               "Content-Length: %3\r\n"
                               "Connection: keep-alive\r\n"
                               "\r\n")
                           .arg(statusInt)
                           .arg(statusText)
                           .arg(bodyUtf8.length());

    pSocket->write(response.toUtf8());
    pSocket->write(bodyUtf8);
    pSocket->waitForBytesWritten(1000);
}

QString HttpResponse::GetStatusText(StatusCode code)
{
    switch (code)
    {
        case StatusCode::OK: 
            return "OK";
        case StatusCode::Created: 
            return "Created";
        case StatusCode::BadRequest: 
            return "Bad Request";
        case StatusCode::NotFound: 
            return "Not Found";
        case StatusCode::InternalServerError: 
            return "Internal Server Error";
        default: 
            return "Unknown";
    }
}
