// Copyright 2018 Florian Muecke. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE.txt file.

#include "MainWindow.h"
#include "ui_MainWindow.h"

#include "../base/FightCategoryManager.h"
#include "../base/FightCategoryManagerDlg.h"
#include "../base/FighterManagerDlg.h"
#include "../base/View.h"
#include "../core/Controller.h"
#include "../api/ApiServer.h"
#include "../api/FightDataDispatcher.h"

#include "../core/Fighter.h"

#include <QColorDialog>
#include <QComboBox>
#include <QCompleter>
#include <QDesktopServices>
#include <QDesktopWidget>
#include <QDir>
#include <QFileDialog>
#include <QFontDialog>
#include <QInputDialog>
#include <QMenu>
#include <QMessageBox>
#include <QSettings>
#include <QTimer>
#include <QUrl>
#include <QDebug>

namespace StrTags
{
static const char* const edition = "Basic Edition";
}

using namespace FMlib;
using namespace Ipponboard;

MainWindow::MainWindow(QWidget* parent)
	: MainWindowBase(parent)
	, m_pUi(new Ui::MainWindow)
	, m_pCategoryManager()
{
	m_pUi->setupUi(this);
}

MainWindow::~MainWindow()
{
}

void MainWindow::Init()
{
	m_pCategoryManager.reset(new FightCategoryMgr());

	MainWindowBase::Init();

	if (m_pApiServer)
	{
		connect(m_pApiServer.get(), &Ipponboard::ApiServer::fightersAdded,
				this, &MainWindow::onFightReceived);
	}

	// init tournament classes (if there are none present)
	for (int i(0); i < m_pCategoryManager->CategoryCount(); ++i)
	{
		FightCategory t("");
		m_pCategoryManager->GetCategory(i, t);
		m_pUi->comboBox_weight_class->addItem(t.ToString());
	}

	// trigger tournament class combobox update
	on_comboBox_weight_class_currentIndexChanged(m_pUi->comboBox_weight_class->currentText());

	m_pUi->actionAutoAdjustPoints->setChecked(m_pController->IsAutoAdjustPoints());
}

void MainWindow::on_button_send_clicked()
{
    if (m_pDispatcher)
    {
        m_pDispatcher->ManualDispatch();
    }
}

void MainWindow::on_button_reload_clicked()
{
    m_pController->DoAction(Ipponboard::eAction_ResetAll, Ipponboard::FighterEnum::Nobody, false);
}

void MainWindow::clear_fighter_metadata()
{
    const QSignalBlocker b1(m_pUi->comboBox_name_first);
    const QSignalBlocker b2(m_pUi->comboBox_name_second);
    const QSignalBlocker b3(m_pUi->comboBox_weight_class);
    const QSignalBlocker b4(m_pUi->comboBox_weight);
    m_pUi->comboBox_name_first->setCurrentIndex(-1);
    m_pUi->comboBox_name_first->setCurrentText("");
    m_pUi->comboBox_name_second->setCurrentIndex(-1);
    m_pUi->comboBox_name_second->setCurrentText("");
    m_pUi->comboBox_weight_class->setCurrentIndex(-1);
    m_pUi->comboBox_weight_class->setCurrentText("");
    m_pUi->comboBox_weight->setCurrentIndex(-1);
    m_pUi->comboBox_weight->setCurrentText("");

    m_pController->SetFighterName(Ipponboard::FighterEnum::First, QString());
    m_pController->SetFighterName(Ipponboard::FighterEnum::Second, QString());
    m_pController->SetWeightClass(QString());
    m_pPrimaryView->UpdateView();
    m_pSecondaryView->UpdateView();
}

void MainWindow::on_actionManageCategories_triggered()
{
	//save categories before editing
	m_pCategoryManager->SaveCategories(); 

	FightCategoryManagerDlg dlg(m_pCategoryManager, this);

	if (QDialog::Accepted == dlg.exec())
	{
		m_pCategoryManager->SaveCategories();

		QString currentClass =
			m_pUi->comboBox_weight_class->currentText();

		m_pUi->comboBox_weight_class->clear();

		for (int i(0); i < m_pCategoryManager->CategoryCount(); ++i)
		{
			FightCategory t("");
			m_pCategoryManager->GetCategory(i, t);
			m_pUi->comboBox_weight_class->addItem(t.ToString());
		}

		int index = m_pUi->comboBox_weight_class->findText(currentClass);

		if (-1 == index)
		{
			index = 0;
			currentClass = m_pUi->comboBox_weight_class->itemText(index);
		}

		m_pUi->comboBox_weight_class->setCurrentIndex(index);
		on_comboBox_weight_class_currentIndexChanged(currentClass);
	}
	else
	{
		//load old categories to discard changes
		m_pCategoryManager->LoadCategories();
	}
}

void MainWindow::on_actionManageFighters_triggered()
{
	MainWindowBase::on_actionManageFighters_triggered();

	FighterManagerDlg dlg(m_fighterManager, this);
	dlg.exec();
}

void MainWindow::on_comboBox_weight_currentIndexChanged(const QString& s)
{
	update_fighter_name_completer(s);

	m_pPrimaryView->SetWeight(s);
	m_pSecondaryView->SetWeight(s);
	m_pPrimaryView->UpdateView();
	m_pSecondaryView->UpdateView();
}

void MainWindow::on_comboBox_name_first_activated(const QString& s)
{
	update_fighters(s);
	m_pController->SetFighterName(FighterEnum::First, s);
	m_pPrimaryView->UpdateView();
	m_pSecondaryView->UpdateView();
}

void MainWindow::on_comboBox_name_second_activated(const QString& s)
{
	update_fighters(s);
	m_pController->SetFighterName(FighterEnum::Second, s);
	m_pPrimaryView->UpdateView();
	m_pSecondaryView->UpdateView();
}

void MainWindow::on_checkBox_golden_score_clicked(bool checked)
{
	const QString name = m_pUi->comboBox_weight_class->currentText();
	FightCategory t(name);
	m_pCategoryManager->GetCategory(name, t);

	m_pController->SetGoldenScore(checked);
	//> Set this before setting the time.
	//> Setting time will then update the views.

	if (checked)
	{
		if (m_pController->GetRules()->IsOption_OpenEndGoldenScore())
		{
			m_pController->SetRoundTime(QTime(0,0,0,0));
		}
		else
		{
			m_pController->SetRoundTime(QTime(0,0,0,0).addSecs(t.GetGoldenScoreTime()));
		}
	}
	else
	{
		m_pController->SetRoundTime(QTime(0,0,0,0).addSecs(t.GetRoundTime()));
	}
}

void MainWindow::on_comboBox_weight_class_currentIndexChanged(const QString& s)
{
	FightCategory category(s);
	m_pCategoryManager->GetCategory(s, category);

	// add weights
	m_pUi->comboBox_weight->clear();
	m_pUi->comboBox_weight->addItems(category.GetWeightsList());

	// trigger round time update
	on_checkBox_golden_score_clicked(m_pUi->checkBox_golden_score->checkState());

	// SAFEGUARD: Don't override if category lookup resulted in 0 seconds
	if (category.GetRoundTime() > 0)
	{
		m_pController->OverrideRoundTimeOfFightMode(category.GetRoundTime());
	}
	else
	{
		qDebug() << "Category" << s << "has 0s round time, keeping previous setting.";
	}
	m_pController->DoAction(Ipponboard::eAction_ResetAll);

	m_pPrimaryView->SetCategory(s);
	m_pSecondaryView->SetCategory(s);
	m_pPrimaryView->UpdateView();
	m_pSecondaryView->UpdateView();
}

void MainWindow::update_fighter_name_completer(const QString& weight)
{
	// Filterung der Kämpfer nach Gewicht UND Kategorie
	m_CurrentFighterNames.clear();
	const QString category = m_pUi->comboBox_weight_class->currentText();

	// Momentane Auswahl speichern ("retten")
	QString currentFirst = m_pUi->comboBox_name_first->currentText();
	QString currentSecond = m_pUi->comboBox_name_second->currentText();

	for (const Ipponboard::Fighter & f : m_fighterManager.m_fighters)
	{
		bool categoryMatch = f.category == category || f.category.isEmpty();
		bool weightMatch = f.weight == weight || f.weight.isEmpty();

		if (categoryMatch && weightMatch)
		{
			const QString fullName =
				QString("%1 %2").arg(f.first_name, f.last_name);

			m_CurrentFighterNames.push_back(fullName);
		}
	}

	m_CurrentFighterNames.sort();
	m_pUi->comboBox_name_first->blockSignals(true);
	m_pUi->comboBox_name_first->clear();
	m_pUi->comboBox_name_first->addItems(m_CurrentFighterNames);
	m_pUi->comboBox_name_first->setCurrentText(currentFirst);
	m_pUi->comboBox_name_first->blockSignals(false);

	m_pUi->comboBox_name_second->blockSignals(true);
	m_pUi->comboBox_name_second->clear();
	m_pUi->comboBox_name_second->addItems(m_CurrentFighterNames);
	m_pUi->comboBox_name_second->setCurrentText(currentSecond);
	m_pUi->comboBox_name_second->blockSignals(false);
}

void MainWindow::update_fighters(const QString& s)
{
	if (s.isEmpty())
		return;

	QString firstName = s;
	QString lastName;

	// Wir suchen das LETZTE Leerzeichen für die Trennung
	int pos = s.lastIndexOf(' ');

	if (pos != -1)
	{
		firstName = s.left(pos).trimmed();
		lastName = s.mid(pos + 1).trimmed();
	}
	else
	{
		firstName = "";
		lastName = s.trimmed();
	}

	const QString weight = m_pUi->comboBox_weight->currentText();
	const QString club; // TODO: later
	const QString category = m_pUi->comboBox_weight_class->currentText();

	Ipponboard::Fighter fNew(firstName, lastName);
	fNew.club = club;
	fNew.weight = weight;
	fNew.category = category;

	m_fighterManager.AddFighter(fNew); // only adds fighter if new
}

void MainWindow::update_statebar()
{
	MainWindowBase::update_statebar();

//    if (Gamepad::eState_ok != m_pGamepad->GetState())
//    {
//        m_pUi->label_controller_state->setText(tr("No controller detected!"));
//    }
//    else
//    {
//        QString controllerName = QString::fromWCharArray(m_pGamepad->GetProductName());
//        m_pUi->label_controller_state->setText(tr("Using controller %1").arg(controllerName));
//    }
	ui_check_rules_items();

//    if (m_pController->GetOption(eOption_Use2013Rules))
//    {
//        m_pUi->actionRulesClassic->setChecked(false);
//        m_pUi->actionRules2013->setChecked(true);
//        m_pUi->actionRules2017->setChecked(false);
//    }
//    else
//    {
//        m_pUi->actionRulesClassic->setChecked(true);
//        m_pUi->actionRules2013->setChecked(false);
//        m_pUi->actionRules2017->setChecked(false);
//    }
}

void MainWindow::attach_primary_view()
{
	QWidget* widget = dynamic_cast<QWidget*>(m_pPrimaryView.get());

	if (widget)
	{
		m_pUi->verticalLayout_3->insertWidget(0, widget, 0);
	}
}

void MainWindow::retranslate_Ui()
{
	m_pUi->retranslateUi(this);
}

void MainWindow::ui_check_language_items()
{
	m_pUi->actionLang_Deutsch->setChecked("de" == m_Language);
	m_pUi->actionLang_English->setChecked("en" == m_Language);
	m_pUi->actionLang_Dutch->setChecked("nl" == m_Language);

	// don't forget second implementation!
}

void MainWindow::ui_check_rules_items()
{
	auto rules = m_pController->GetRules();
	m_pUi->actionRulesClassic->setChecked(rules->IsOfType<ClassicRules>());
	m_pUi->actionRules2013->setChecked(rules->IsOfType<Rules2013>());
	m_pUi->actionRules2017->setChecked(rules->IsOfType<Rules2017>());
	m_pUi->actionRules2017U15->setChecked(rules->IsOfType<Rules2017U15>());
	m_pUi->actionRules2018->setChecked(rules->IsOfType<Rules2018>());
	m_pUi->actionRules2025->setChecked(rules->IsOfType<Rules2025>());
	m_pUi->actionRulesPfalz->setChecked(rules->IsOfType<RulesPfalzU13>());

	if (rules->IsOfType<ClassicRules>())
	{
		m_pUi->label_usedRules->setText(m_pUi->actionRulesClassic->text());
	}
	else if (rules->IsOfType<Rules2013>())
	{
		m_pUi->label_usedRules->setText(m_pUi->actionRules2013->text());
	}
	else if (rules->IsOfType<Rules2017>())
	{
		m_pUi->label_usedRules->setText(m_pUi->actionRules2017->text());
	}
	else if (rules->IsOfType<Rules2017U15>())
	{
		m_pUi->label_usedRules->setText(m_pUi->actionRules2017U15->text());
	}
	else if (rules->IsOfType<Rules2018>())
	{
		m_pUi->label_usedRules->setText(m_pUi->actionRules2018->text());
	}
	else if (rules->IsOfType<Rules2025>())
	{
		m_pUi->label_usedRules->setText(m_pUi->actionRules2025->text());
	}
	else if (rules->IsOfType<RulesPfalzU13>())
	{
		m_pUi->label_usedRules->setText(m_pUi->actionRulesPfalz->text());
	}

	m_pPrimaryView->UpdateView();
	m_pSecondaryView->UpdateView();
}

void MainWindow::ui_check_show_secondary_view(bool checked) const
{
	m_pUi->actionShow_SecondaryView->setChecked(checked);
	m_pUi->toolButton_viewSecondaryScreen->setChecked(checked);
}

void MainWindow::on_actionAutoAdjustPoints_toggled(bool checked)
{
	MainWindowBase::on_actionAutoAdjustPoints_toggled(checked);

	ui_update_used_options();
}

void MainWindow::ui_update_used_options()
{
	QString text;

	if (m_pController->IsAutoAdjustPoints())
	{
		text += tr("Auto adjust points");
	}
	else
	{
		text = "-";
	}

	m_pUi->label_usedOptions->setText(text);

}

void MainWindow::on_actionViewInfoBar_toggled(bool checked)
{
	for (int i = 0; i < m_pUi->horizontalLayout_infoBar->count(); ++i)
	{
		auto item = m_pUi->horizontalLayout_infoBar->itemAt(i)->widget();

		if (item)
		{
			if (checked)
			{
				item->show();
			}
			else
			{
				item->hide();
			}
		}
	}

	if (checked)
	{
		m_pUi->line_infoBar->show();
	}
	else
	{
		m_pUi->line_infoBar->hide();
	}
}

void MainWindow::on_toolButton_viewSecondaryScreen_toggled()
{
	on_actionShow_SecondaryView_triggered();
}

void MainWindow::write_specific_settings(QSettings& settings)
{
	settings.beginGroup(EditionNameShort());
	{
		settings.remove("");
		settings.setValue(str_tag_MatLabel, m_MatLabel);
		settings.setValue(str_tag_rules, m_pController->GetRules()->Name());
		settings.setValue(str_tag_rulesByAgeGroup, m_rulesByAgeGroup);
	}
	settings.endGroup();
}

void MainWindow::read_specific_settings(QSettings& settings)
{
	settings.beginGroup(EditionNameShort());
	{
        m_MatLabel = settings.value(str_tag_MatLabel, "Ipponboard").toString(); // value is also in settings dialog!
		m_pPrimaryView->SetMat(m_MatLabel);
		m_pSecondaryView->SetMat(m_MatLabel);

		// rules
		auto rules = RulesFactory::Create(settings.value(str_tag_rules).toString());
		m_pController->SetRules(rules);

		// age-group -> ruleset map for the POST /fighters auto-switch (configurable)
		m_rulesByAgeGroup = settings.value(str_tag_rulesByAgeGroup,
										   Ipponboard::DefaultRulesByAgeGroupSpec()).toString();
	}
	settings.endGroup();
}

void MainWindow::UpdateGoldenScoreView()
{
	m_pUi->checkBox_golden_score->setEnabled(m_pController->GetRules()->IsOption_OpenEndGoldenScore());
	m_pUi->checkBox_golden_score->setChecked(m_pController->IsGoldenScore());
}

void MainWindow::SelectCategory(const QString& category)
{
	auto* combo = m_pUi->comboBox_weight_class;

	// Resolve a (possibly raw) API label to the canonical catalogue text: exact
	// first, then case-insensitive (e.g. "mU15" -> "MU15").
	auto resolve = [combo](const QString& name) -> int
	{
		int idx = combo->findText(name, Qt::MatchExactly);
		if (idx == -1)
		{
			for (int i = 0; i < combo->count(); ++i)
			{
				if (combo->itemText(i).compare(name, Qt::CaseInsensitive) == 0)
				{
					idx = i;
					break;
				}
			}
		}
		return idx;
	};

	int indexCategory = resolve(category);

	// Gender-neutral age groups diverge between the repos: Ipponboard's catalogue
	// stores U9/U11 without a gender prefix while gendered groups are M/F-prefixed,
	// and JudgeFrontend treats U11/U13 as gender-neutral. If the gender-prefixed
	// label ("MU11") doesn't resolve, retry with the leading M/F/W marker stripped
	// so a neutral catalogue entry ("U11") still matches. The reverse (JF-neutral
	// "U13" vs catalogue-gendered "MU13/FU13") stays unresolved by design and falls
	// through to the graceful "round time unchanged" path in onFightReceived.
	if (indexCategory == -1 && category.size() > 1)
	{
		const QChar lead = category.at(0).toLower();
		if (lead == QChar('m') || lead == QChar('f') || lead == QChar('w'))
		{
			indexCategory = resolve(category.mid(1));
		}
	}

	QString finalCategory = category;
	if (indexCategory != -1)
	{
		combo->setCurrentIndex(indexCategory);
		finalCategory = combo->itemText(indexCategory);
	}
	else
	{
		combo->setCurrentText(category);
	}

	// Trigger category logic (fills the weight list)
	on_comboBox_weight_class_currentIndexChanged(finalCategory);
}

void MainWindow::SelectWeight(const QString& weightClass)
{
	int indexWeight = m_pUi->comboBox_weight->findText(weightClass, Qt::MatchExactly);
	if (indexWeight == -1)
	{
		QString simpleWeight = weightClass;
		simpleWeight = simpleWeight.remove("kg", Qt::CaseInsensitive).trimmed();

		for (int i = 0; i < m_pUi->comboBox_weight->count(); ++i)
		{
			QString itemText = m_pUi->comboBox_weight->itemText(i);
			if (itemText.startsWith(simpleWeight, Qt::CaseInsensitive) ||
				itemText.contains(simpleWeight, Qt::CaseInsensitive))
			{
				indexWeight = i;
				break;
			}
		}
	}

	if (indexWeight != -1)
	{
		m_pUi->comboBox_weight->setCurrentIndex(indexWeight);
	}
	else
	{
		m_pUi->comboBox_weight->addItem(weightClass);
		m_pUi->comboBox_weight->setCurrentText(weightClass);
	}

	// Trigger weight logic
	on_comboBox_weight_currentIndexChanged(weightClass);
}

void MainWindow::SetFighters(const QString& fighter1Name, const QString& fighter2Name)
{
	m_pUi->comboBox_name_first->setCurrentText(fighter1Name);
	m_pUi->comboBox_name_second->setCurrentText(fighter2Name);

	// Trigger fighter update logic
	on_comboBox_name_first_activated(fighter1Name);
	on_comboBox_name_second_activated(fighter2Name);
}

void MainWindow::onFightReceived(const QString& category, const QString& weightClass, const QString& fighter1Name, const QString& fighter2Name, const QString& pool)
{
	// Block signals to prevent flickering/recursion while we set multiple values
	m_pUi->comboBox_weight_class->blockSignals(true);
	m_pUi->comboBox_weight->blockSignals(true);
	m_pUi->comboBox_name_first->blockSignals(true);
	m_pUi->comboBox_name_second->blockSignals(true);

	SelectCategory(category);
	SelectWeight(weightClass);

	// Optional pool label (e.g. "Pool 3") — shown verbatim in the info bar; empty -> nothing.
	// Set before SetFighters so its UpdateView renders the pool in every branch.
	m_pPrimaryView->SetPool(pool);
	m_pSecondaryView->SetPool(pool);

	SetFighters(fighter1Name, fighter2Name);

	// Auto-apply the round time of the received fight's category so the correct
	// fight time (and golden-score time, which the GS toggle reads fresh from the
	// category) is in effect. The weight/category combo-box change handlers do NOT
	// touch the clock, so without this the time stays at the mode-load value.
	const QString resolvedCategory = m_pUi->comboBox_weight_class->currentText();
	FightCategory cat(resolvedCategory);
	if (m_pCategoryManager->GetCategory(resolvedCategory, cat) && cat.GetRoundTime() > 0)
	{
		m_pController->SetRoundTime(QTime(0, 0, 0, 0).addSecs(cat.GetRoundTime()));
	}
	else
	{
		qWarning() << "POST /fighters: no FightCategory match for" << resolvedCategory
				   << "- round time unchanged";
	}

	// Auto-select the ruleset for the received age group (U9/U11 -> JVP additive,
	// everything else -> IJF), driven by the configurable Ipponboard.ini
	// RulesByAgeGroup map. Empty/unresolved age group -> leave the ruleset as is.
	const QString ageGroup = AgeGroupFromCategory(resolvedCategory);
	if (!ageGroup.isEmpty())
	{
		const QString wantRules = RulesetForAgeGroup(
			ParseRulesByAgeGroup(m_rulesByAgeGroup), ageGroup, Rules2025::StaticName);
		if (wantRules != m_pController->GetRules()->Name())
		{
			m_pController->SetRules(RulesFactory::Create(wantRules));
		}
	}

	// Operational trace of what the received fight actually applied (category resolve,
	// fight time, ruleset auto-switch, pool label) — useful when debugging at a live mat.
	qInfo() << "POST /fighters applied:"
			<< "category=" << resolvedCategory
			<< "ageGroup=" << ageGroup
			<< "ruleset=" << m_pController->GetRules()->Name()
			<< "roundTime=" << m_pController->GetFightTimeString()
			<< "pool=" << (pool.isEmpty() ? QStringLiteral("-") : pool);

	// Unblock signals
	m_pUi->comboBox_weight_class->blockSignals(false);
	m_pUi->comboBox_weight->blockSignals(false);
	m_pUi->comboBox_name_first->blockSignals(false);
	m_pUi->comboBox_name_second->blockSignals(false);
}
