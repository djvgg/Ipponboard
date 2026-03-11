#ifndef API__HTTPRESPONSE_H_
#define API__HTTPRESPONSE_H_

#include <QString>

class QTcpSocket;

namespace Ipponboard
{

class HttpResponse
{
public:
    enum class StatusCode
    {
        OK = 200,
        Created = 201,
        BadRequest = 400,
        NotFound = 404,
        InternalServerError = 500
    };

    static void Send(QTcpSocket* pSocket, StatusCode code, const QString& body = "");

private:
    static QString GetStatusText(StatusCode code);
};

}

#endif // API__HTTPRESPONSE_H_
