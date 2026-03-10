#ifndef API__APISERVER_H_
#define API__APISERVER_H_

#include <QTcpServer>

namespace Ipponboard
{
class Controller;

constexpr uint16_t PORT = 8080;

class ApiServer : public QTcpServer
{
    Q_OBJECT

public:
    explicit ApiServer(Controller* pController, QObject* parent = nullptr);
    ~ApiServer();

    bool StartListening(uint16_t port);
    void StopListening();

protected:
    void incomingConnection(qintptr socketDescriptor) override;

private:
    Controller* m_pController;
};

}

#endif // API__APISERVER_H_
