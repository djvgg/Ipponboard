#include "HttpResponse.h"
#include <QTcpSocket>

using namespace Ipponboard;

void HttpResponse::Send(QTcpSocket* pSocket, StatusCode code, const QString& body)
{
    int statusInt = static_cast<int>(code);
    QString statusText = GetStatusText(code);

    QString response = QString("HTTP/1.1 %1 %2\r\n"
                               "Content-Type: text/plain\r\n"
                               "\r\n"
                               "%3")
                           .arg(statusInt)
                           .arg(statusText)
                           .arg(body);

    pSocket->write(response.toUtf8()); // Antwort senden
    pSocket->waitForBytesWritten(1000);
    pSocket->disconnectFromHost();
}

QString HttpResponse::GetStatusText(StatusCode code)
{
    switch (code)
    {
        case StatusCode::OK:                  return "OK";
        case StatusCode::Created:             return "Created";
        case StatusCode::BadRequest:          return "Bad Request";
        case StatusCode::NotFound:            return "Not Found";
        case StatusCode::InternalServerError: return "Internal Server Error";
        default:                              return "Unknown";
    }
}
