#include "ApiEndpoints.h"
#include "HttpResponse.h"
#include <QTcpSocket>
#include <QJsonObject>
#include <iostream>

using namespace Ipponboard;

ApiEndpoints::ApiEndpoints(Controller* pController)
    : m_pController(pController)
{
}

void ApiEndpoints::HandlePostFighter(QTcpSocket* pSocket, const QJsonObject& json)
{
    // Felder aus dem JSON lesen
    QString firstName = json["firstname"].toString();
    QString lastName = json["lastname"].toString();
    QString weight = json["weightclass"].toString();
    QString gender = json["gender"].toString(); 
    QString ageGroup = json["agegroup"].toString();
    
    // Konkateniere aus "M" und "U18" den internen Ipponboard String "MU18"
    QString category = gender + ageGroup;

    std::cout << "Fighter received: " << firstName.toStdString()
              << " " << lastName.toStdString()
              << " | " << category.toStdString()
              << " | " << weight.toStdString() << std::endl;

    // TODO: Fighter an den Controller übergeben
    // m_pController->...

    HttpResponse::Send(pSocket, HttpResponse::StatusCode::Created, "Fighter created");
}
