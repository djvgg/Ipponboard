#include "ApiEndpoints.h"
#include "HttpResponse.h"
#include "../core/Fighter.h"
#include "../base/FighterManager.h"
#include <QTcpSocket>
#include <QJsonObject>
#include <QJsonDocument>
#include <iostream>

using namespace Ipponboard;

ApiEndpoints::ApiEndpoints(Controller* pController, FighterManager* pFighterManager)
    : m_pController(pController)
    , m_pFighterManager(pFighterManager)
{
}

PostFightersResult ApiEndpoints::HandlePostFighters(QTcpSocket* pSocket, const QJsonObject& json)
{
    PostFightersResult result;

    // DEBUG: Zeige das komplette empfangene JSON als Raw-String
    QJsonDocument debugDoc(json);
    std::cout << "========== RAW JSON RECEIVED ==========" << std::endl;
    std::cout << debugDoc.toJson(QJsonDocument::Indented).toStdString() << std::endl;
    std::cout << "=======================================" << std::endl;

    // Prüfen ob beide Fighter-Objekte im JSON vorhanden sind
    if (!json.contains("fighter1") || !json["fighter1"].isObject())
    {
        HttpResponse::Send(pSocket, HttpResponse::StatusCode::BadRequest, "Bad Request: 'fighter1' object missing");
        return result;
    }

    if (!json.contains("fighter2") || !json["fighter2"].isObject())
    {
        HttpResponse::Send(pSocket, HttpResponse::StatusCode::BadRequest, "Bad Request: 'fighter2' object missing");
        return result;
    }

    // Beide Fighter aus ihren Unter-Objekten parsen
    Ipponboard::Fighter fighter1 = ParseFighterFromJson(json["fighter1"].toObject());
    Ipponboard::Fighter fighter2 = ParseFighterFromJson(json["fighter2"].toObject());

    std::cout << "Fighter 1 received: " << fighter1.first_name.toStdString()
              << " " << fighter1.last_name.toStdString()
              << " | " << fighter1.category.toStdString()
              << " | " << fighter1.weight.toStdString() << std::endl;

    std::cout << "Fighter 2 received: " << fighter2.first_name.toStdString()
              << " " << fighter2.last_name.toStdString()
              << " | " << fighter2.category.toStdString()
              << " | " << fighter2.weight.toStdString() << std::endl;

    // Beide Fighter dem FighterManager hinzufügen
    if (m_pFighterManager)
    {
        m_pFighterManager->AddFighter(fighter1);
        m_pFighterManager->AddFighter(fighter2);
    }

    HttpResponse::Send(pSocket, HttpResponse::StatusCode::Created, "Both fighters created");

    // Ergebnis für die UI befüllen
    result.success = true;
    result.category = fighter1.category; // Beide Fighter sollten die gleiche Kategorie haben
    result.weightClass = fighter1.weight; // Beide Fighter sollten das gleiche Gewicht haben
    result.fighter1Name = QString("%1 %2").arg(fighter1.first_name, fighter1.last_name);
    result.fighter2Name = QString("%1 %2").arg(fighter2.first_name, fighter2.last_name);

    return result;
}

Ipponboard::Fighter ApiEndpoints::ParseFighterFromJson(const QJsonObject& fighterJson)
{
    QString firstName = fighterJson["firstname"].toString();
    QString lastName  = fighterJson["lastname"].toString();
    QString weight    = fighterJson["weightclass"].toString();
    QString gender    = fighterJson["gender"].toString();
    QString ageGroup  = fighterJson["agegroup"].toString();

    // Konkateniere aus "M" und "U18" den internen Ipponboard String "MU18"
    QString category = gender + ageGroup;

    Ipponboard::Fighter fighter(firstName, lastName);
    fighter.weight = weight;
    fighter.category = category;

    return fighter;
}
