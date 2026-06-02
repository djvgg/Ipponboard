// Copyright 2018 Florian Muecke. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE.txt file.

#ifndef BASE__RULESBYAGEGROUP_H_
#define BASE__RULESBYAGEGROUP_H_

#include <QString>
#include <QStringList>
#include <QChar>
#include <utility>
#include <vector>

// Age-group -> ruleset mapping for the POST /fighters auto-switch. The map is
// operator-configurable in Ipponboard.ini (key RulesByAgeGroup, format mirrors
// FightTimeOverrides: "U9:JVP-Additiv-20;U11:JVP-Additiv-20"). Header-only so it
// is unit-testable without MainWindow's Qt-widget dependencies. See WSP/CLAUDE.md
// "Ipponboard also auto-switches the RULESET per age group on receive"
// (Decision 2026-05-30).
namespace Ipponboard
{

// Default mapping (Merlin's rule): U9 + U11 run the Pfalz additive ruleset, every
// other age group falls back to IJF (see RulesetForAgeGroup's defaultRuleset).
inline const char* DefaultRulesByAgeGroupSpec()
{
    return "U9:JVP-Additiv-20;U11:JVP-Additiv-20";
}

// Derive the age group from a resolved category name by stripping a leading
// gender marker: "MU15" -> "U15", "F18+" -> "18+", "U11" -> "U11", "" -> "".
inline QString AgeGroupFromCategory(const QString& category)
{
    const QString c = category.trimmed();
    if (c.size() > 1)
    {
        const QChar lead = c.at(0).toLower();
        if (lead == QChar('m') || lead == QChar('f') || lead == QChar('w'))
            return c.mid(1);
    }
    return c;
}

// Parse "U9:JVP-Additiv-20;U11:JVP-Additiv-20" into [(U9, JVP-Additiv-20), ...].
// Splits each entry at the FIRST colon so ruleset names keep their own hyphens.
// Malformed entries (no colon, empty key/value) are skipped.
inline std::vector<std::pair<QString, QString>> ParseRulesByAgeGroup(const QString& spec)
{
    std::vector<std::pair<QString, QString>> result;
    const QStringList entries = spec.split(';', Qt::SkipEmptyParts);
    for (const QString& entry : entries)
    {
        const int colon = entry.indexOf(':');
        if (colon <= 0)
            continue;
        const QString key = entry.left(colon).trimmed();
        const QString val = entry.mid(colon + 1).trimmed();
        if (!key.isEmpty() && !val.isEmpty())
            result.emplace_back(key, val);
    }
    return result;
}

// Ruleset name for the given age group: the mapped value if listed (case-insensitive),
// otherwise defaultRuleset. Callers must skip an empty ageGroup themselves (leave the
// current ruleset untouched for QUARANTINE / unresolved categories).
inline QString RulesetForAgeGroup(const std::vector<std::pair<QString, QString>>& map,
                                  const QString& ageGroup,
                                  const QString& defaultRuleset)
{
    for (const auto& kv : map)
    {
        if (kv.first.compare(ageGroup, Qt::CaseInsensitive) == 0)
            return kv.second;
    }
    return defaultRuleset;
}

} // namespace Ipponboard

#endif // BASE__RULESBYAGEGROUP_H_
