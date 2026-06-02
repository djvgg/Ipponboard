#ifndef API__APIENDPOINTS_H_
#define API__APIENDPOINTS_H_

#include <QString>

class QTcpSocket;
class QJsonObject;

namespace Ipponboard
{
class Controller;
class FighterManager;

class Fighter;

// Map the (inconsistent) wire gender labels onto Ipponboard's FightCategory
// naming convention (M / F). The upstream values are NOT normalized producer-side
// (edv export / contestants_*.json mixes "m"/"male"/"female", no "w"), so the
// normalization lives here at the API boundary — see WSP/CLAUDE.md
// "Ipponboard owns category<->fight-time mapping". Header-only so it is
// unit-testable without the ApiEndpoints translation unit's dependencies.
// Matches on the leading letter to stay locale-/encoding-proof (covers
// m/male/männlich and w/weiblich/f/female). Unknown/empty -> passed through.
inline QString NormalizeGenderLabel(const QString& gender)
{
    const QString g = gender.trimmed();
    if (g.isEmpty())
        return g;

    const QChar c = g.at(0).toLower();
    if (c == QChar('m'))
        return QStringLiteral("M");
    if (c == QChar('w') || c == QChar('f'))
        return QStringLiteral("F");

    return g;
}

// Build the FightCategory lookup key (e.g. "MU15") from the wire fields.
inline QString BuildCategoryName(const QString& gender, const QString& ageGroup)
{
    return NormalizeGenderLabel(gender) + ageGroup.trimmed();
}

struct PostFightersResult
{
    bool success = false;
    QString category;
    QString weightClass;
    QString fighter1Name;
    QString fighter2Name;
    QString pool;          // optional, display-ready (e.g. "Pool 3"); empty if not a pool fight
    QString callbackUrl;
};

class ApiEndpoints
{
public:
    explicit ApiEndpoints(Controller* pController, FighterManager* pFighterManager);

    PostFightersResult HandlePostFighters(QTcpSocket* pSocket, const QJsonObject& json);

private:
    Fighter ParseFighterFromJson(const QJsonObject& fighterJson);

    Controller* m_pController;
    FighterManager* m_pFighterManager;
};

}

#endif // API__APIENDPOINTS_H_
