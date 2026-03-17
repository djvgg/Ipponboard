#include "ApiEndpoints.h"
#include "HttpResponse.h"
#include "../core/Fighter.h"
#include "../base/FighterManager.h"
#include <QTcpSocket>
#include <QJsonDocument>
#include <QString>
#include <QJsonObject>
#include <QDebug>

using namespace Ipponboard;

ApiEndpoints::ApiEndpoints(Controller* pController, FighterManager* pFighterManager)
    : m_pController(pController)
    , m_pFighterManager(pFighterManager)
{
}

PostFightersResult ApiEndpoints::HandlePostFighters(QTcpSocket* pSocket, const QJsonObject& json)
{
    PostFightersResult result;

    qInfo() << "Request: POST /fighters (Body length:" << QJsonDocument(json).toJson().length() << ")";

    if (!json.contains("fighter1") || !json.value("fighter1").isObject())
    {
        HttpResponse::Send(pSocket, HttpResponse::StatusCode::BadRequest, "Bad Request: 'fighter1' object missing");
        return result;
    }

    if (!json.contains("fighter2") || !json.value("fighter2").isObject())
    {
        HttpResponse::Send(pSocket, HttpResponse::StatusCode::BadRequest, "Bad Request: 'fighter2' object missing");
        return result;
    }

    // Parse fighters from their sub-objects
    Ipponboard::Fighter fighter1 = ParseFighterFromJson(json.value("fighter1").toObject());
    Ipponboard::Fighter fighter2 = ParseFighterFromJson(json.value("fighter2").toObject());

    qInfo() << "Fighter 1 received:" << fighter1.first_name << fighter1.last_name << "|" << fighter1.category << "|" << fighter1.weight;
    qInfo() << "Fighter 2 received:" << fighter2.first_name << fighter2.last_name << "|" << fighter2.category << "|" << fighter2.weight;

    // Add fighters to the manager
    if (m_pFighterManager)
    {
        m_pFighterManager->AddFighter(fighter1);
        m_pFighterManager->AddFighter(fighter2);
    }

    HttpResponse::Send(pSocket, HttpResponse::StatusCode::Created, "Both fighters created");

    // Result for the UI
    result.success = true;
    result.category = fighter1.category;
    result.weightClass = fighter1.weight;
    result.fighter1Name = QString("%1 %2").arg(fighter1.first_name, fighter1.last_name);
    result.fighter2Name = QString("%1 %2").arg(fighter2.first_name, fighter2.last_name);
    result.callbackUrl = json.value("callback").toString();

    return result;
}

Ipponboard::Fighter ApiEndpoints::ParseFighterFromJson(const QJsonObject& fighterJson)
{
    QString firstName = fighterJson.value("firstname").toString();
    QString lastName  = fighterJson.value("lastname").toString();
    QString weight    = fighterJson.value("weightclass").toString();
    QString gender    = fighterJson.value("gender").toString();
    QString ageGroup  = fighterJson.value("agegroup").toString();
 
    QString category = gender + ageGroup;
    
    if (category.isEmpty())
    {
        qWarning() << "Received empty fighter category info!";
    }

    Ipponboard::Fighter fighter(firstName, lastName);
    fighter.weight = weight;
    fighter.category = category;

    return fighter;
}
