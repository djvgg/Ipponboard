// Copyright 2018 Florian Muecke. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE.txt file.

#include "View.h"
#include "ui_view_horizontal.h"
#include "../core/Rules.h"

#include <QPainter>
#include <QMessageBox>
#include <QTimer>
#include <QTime>
#include <QFontDatabase>
#include <QMenu>
#include <QInputDialog>

using namespace Ipponboard;
using Point = Score::Point;

//=========================================================
View::View(IController* pController, EditionType edition, EType type, QWidget* parent)
	: QWidget(parent)
	, m_Edition(edition)
	, m_Type(type)
	, m_pController(pController)
	, ui(new Ui::ScoreViewHorizontal)
	, m_InfoHeaderFont("Calibri", 12, QFont::Bold, false)
	, m_FighterNameFont("Calibri", 12, QFont::Bold, false)
	, m_DigitFont("Calibri", 12, QFont::Bold, false)
	, m_TextColorFirst(Qt::black)
	, m_TextBgColorFirst(Qt::white)
	, m_TextColorSecond(Qt::white)
	, m_TextBgColorSecond(Qt::blue)
	, m_InfoTextColor(QColor(255, 255, 96)) //Qt::darkFirst
	, m_InfoTextBgColor(Qt::black) //Qt::lightGray
	, m_MainClockBgColor(Qt::black)
	, m_MainClockColorRunning(Qt::yellow)
	, m_MainClockColorStopped(Qt::red)
	, m_mat(QCoreApplication::applicationName() + " v" +
			QCoreApplication::applicationVersion())
	, m_weight("")
	, m_category("")
	, m_pool("")
	, m_drawIppon(false)
	, m_showInfoHeader(true)
    , m_pBlinkTimer(nullptr)
//=========================================================
{
	// init widgets
	ui->setupUi(this);
	ui->image_shido1_second->UpdateImage(":res/images/off.png");
	ui->image_shido2_second->UpdateImage(":res/images/off.png");
	ui->image_shido3_second->UpdateImage(":res/images/off.png");
	ui->image_shido1_first->UpdateImage(":res/images/off.png");
	ui->image_shido2_first->UpdateImage(":res/images/off.png");
	ui->image_shido3_first->UpdateImage(":res/images/off.png");
	// additive Shido count digit is hidden until update_shido() shows it (operator
	// console, JVP additive ruleset); see update_shido() / set_ijf_lamps_visible()
	ui->text_shido_first->hide();
	ui->text_shido_second->hide();
	// additive 0..20 total next to the clock — operator-console-only, shown by
	// update_operator_total(); hidden until then (see set_ijf_lamps_visible())
	ui->text_total_first->hide();
	ui->text_total_second->hide();
	ui->image_hansokumake_first->UpdateImage(":res/images/off.png");
	ui->image_hansokumake_second->UpdateImage(":res/images/off.png");
	ui->image_sand_clock->UpdateImage(":res/images/sand_clock.png");
	ui->dummy_first->UpdateImage(":res/images/off_empty.png");
	ui->dummy_second->UpdateImage(":res/images/off_empty.png");
	ui->image_golden_score->UpdateImage(":res/images/golden_score.png");
	ui->image_golden_score->SetBgColor(Qt::black);

	ui->text_hold_clock_first->SetText("00", ScaledText::eSize_full);
	ui->text_hold_clock_second->SetText("00", ScaledText::eSize_full);

	QColor bgColor1 = get_color(firstBg);
	QColor fgColor1 = get_color(firstFg);
	QColor bgColor2 = get_color(secondBg);
	QColor fgColor2 = get_color(secondFg);

	// set point descriptions
	const QFont descFont("Calibri", 12, QFont::Bold, false);

	ui->text_ippon_desc1->setFont(descFont);
	ui->text_wazaari_desc1->setFont(descFont);
	ui->text_yuko_desc1->setFont(descFont);
	ui->text_ippon_desc1->SetColor(fgColor1);
	ui->text_wazaari_desc1->SetColor(fgColor1);
	ui->text_yuko_desc1->SetColor(fgColor1);
	ui->text_ippon_desc1->SetText("I");
	ui->text_wazaari_desc1->SetText("W");
	ui->text_yuko_desc1->SetText("Y");
	ui->text_ippon_desc2->setFont(descFont);
	ui->text_wazaari_desc2->setFont(descFont);
	ui->text_yuko_desc2->setFont(descFont);
	ui->text_ippon_desc2->SetColor(fgColor2, bgColor2);
	ui->text_wazaari_desc2->SetColor(fgColor2, bgColor2);
	ui->text_yuko_desc2->SetColor(fgColor2, bgColor2);
	ui->text_ippon_desc2->SetText("I");
	ui->text_wazaari_desc2->SetText("W");
	ui->text_yuko_desc2->SetText("Y");
	ui->text_ippon_desc1->SetColor(fgColor1, bgColor1);
	ui->text_wazaari_desc1->SetColor(fgColor1, bgColor1);
	ui->text_yuko_desc1->SetColor(fgColor1, bgColor1);
	ui->dummy_first->SetBgColor(bgColor1);
	ui->dummy_second->SetBgColor(bgColor2);

	if (m_Edition == EditionType::Team)
	{
		ui->text_score_team_first_label->SetColor(Qt::gray, Qt::black);
		ui->text_score_team_second_label->SetColor(Qt::gray, Qt::black);
		ui->text_score_team_first->SetColor(Qt::gray, Qt::black);
		ui->text_score_team_second->SetColor(Qt::gray, Qt::black);
		ui->layout_info_top->setStretchFactor(ui->text_mat, 3);
		ui->layout_info_top->setStretchFactor(ui->text_weight, 1);
	}

	ui->text_ippon_first->SetColor(fgColor1, bgColor1);
	ui->text_wazaari_first->SetColor(fgColor1, bgColor1);
	ui->text_yuko_first->SetColor(fgColor1, bgColor1);
	ui->text_ippon_second->SetColor(fgColor2, bgColor2);
	ui->text_wazaari_second->SetColor(fgColor2, bgColor2);
	ui->text_yuko_second->SetColor(fgColor2, bgColor2);

	ui->text_lastname_first->SetColor(fgColor1, bgColor1);
	ui->text_firstname_first->SetColor(fgColor1, bgColor1);
	ui->text_lastname_second->SetColor(fgColor2, bgColor2);
	ui->text_firstname_second->SetColor(fgColor2, bgColor2);

	if (is_secondary())
	{
		ui->text_ippon_desc1->hide();
		ui->text_ippon_desc2->hide();
		ui->text_wazaari_desc1->SetText("Waza-ari");
		ui->text_wazaari_desc1->SetText("Waza-ari");
		ui->text_yuko_desc1->SetText("Yuko");
		ui->text_yuko_desc2->SetText("Yuko");
	}
	else
	{
		ui->text_ippon_desc1->show();
		ui->text_ippon_desc2->show();
	}

	ui->image_shido1_first->SetBgColor(bgColor1);
	ui->image_shido2_first->SetBgColor(bgColor1);
	ui->image_shido3_first->SetBgColor(bgColor1);
	ui->image_hansokumake_first->SetBgColor(bgColor1);
	ui->image_shido1_second->SetBgColor(bgColor2);
	ui->image_shido2_second->SetBgColor(bgColor2);
	ui->image_shido3_second->SetBgColor(bgColor2);
	ui->image_hansokumake_second->SetBgColor(bgColor2);

//	QFontDatabase fontDb;
//	QFont newFont = fontDb.font("Bonzai", "Normal", 12 );

	SetInfoHeaderFont(QFont("Calibri", 12, QFont::Bold, false));
	SetDigitFont(QFont("Arial", 12, QFont::Bold, false));

	ui->text_mat->SetColor(m_InfoTextColor, m_InfoTextBgColor);
	ui->text_weight->SetColor(m_InfoTextColor, m_InfoTextBgColor);

	m_pBlinkTimer = new QTimer(this);
	connect(m_pBlinkTimer, SIGNAL(timeout()), this, SLOT(blink_()));
}

//=========================================================
View::~View()
//=========================================================
{
    m_pController = nullptr;
	delete ui;
}

//=========================================================
void View::UpdateView()
//=========================================================
{
	Q_ASSERT(m_pController && "Controller not set!");

	// JVP additive scoring shows one big total on the secondary screen. The
	// per-category indicator visibility below must be skipped in that case,
	// otherwise it re-shows the yuko/shido widgets that the layout transition
	// hid — every frame — leaving a second (yuko) number next to the total.
	const bool wantAdditive = is_secondary()
							  && (m_pController->GetDisplayScore(FighterEnum::First) >= 0);

	if (!wantAdditive)
	{
		if (m_pController->GetRules()->GetMaxShidoCount() < 3)
		{
			ui->image_shido3_first->hide();
			ui->image_shido3_second->hide();
		}
		else
		{
			ui->image_shido3_first->show();
			ui->image_shido3_second->show();
		}

		if (m_pController->GetRules()->IsOption_HasYuko())
		{
			ui->text_yuko_first->show();
			ui->text_yuko_second->show();
			ui->text_yuko_desc1->show();
			ui->text_yuko_desc2->show();
		}
		else
		{
			ui->text_yuko_first->hide();
			ui->text_yuko_second->hide();
			ui->text_yuko_desc1->hide();
			ui->text_yuko_desc2->hide();
		}
	}

	show_golden_score(m_pController->IsGoldenScore());

	update_colors();

	ui->text_mat->SetText(m_mat, ScaledText::eSize_normal);

    // display weight class
    if (m_Edition == EditionType::Team)
	{
		QString infoText/*(tr("Fight ").toUpper())*/;
		//infoText += QString::number(m_pController->GetRound()) + ": ";
		infoText += m_pController->GetWeight();
		ui->text_weight->SetText(infoText, ScaledText::eSize_normal);
	}
	else
	{
		QString infoText(m_category);

		if (!infoText.isEmpty())
            infoText += "  ";

        infoText += m_weight;//.toUpper();

        // optional pool label (e.g. "Pool 3"), shown verbatim; empty -> nothing
        if (!m_pool.isEmpty())
            infoText += "   " + m_pool;

        ui->text_weight->SetText(infoText, ScaledText::eSize_normal);
	}

	if (m_showInfoHeader)
	{
		ui->verticalLayout_main->setStretchFactor(ui->layout_info_top, 4);
		ui->text_weight->setVisible(true);
		ui->text_mat->setVisible(true);
	}
	else
	{
		ui->verticalLayout_main->setStretchFactor(ui->layout_info_top, 0);
		ui->text_weight->setVisible(false);
		ui->text_mat->setVisible(false);
	}

	//
	// fighter names
	//
	auto combinedName = [this](FighterEnum who) {
		const QString last = m_pController->GetFighterLastName(GVF_(who)).toUpper().trimmed();
		const QString first = m_pController->GetFighterFirstName(GVF_(who)).toUpper().trimmed();
		if (last.isEmpty() && first.isEmpty()) return QString();
		if (first.isEmpty()) return last;
		if (last.isEmpty()) return first;
		return last + "\n" + first;
	};

	ui->text_lastname_first->SetText(combinedName(FighterEnum::First), ScaledText::eSize_normal);
	ui->text_lastname_second->SetText(combinedName(FighterEnum::Second), ScaledText::eSize_normal);
	ui->text_firstname_first->setVisible(false);
	ui->text_firstname_second->setVisible(false);

	// the secondary screen shows the additive total (wantAdditive, computed at
	// the top of UpdateView); the KR/primary view keeps the IJF lamps
	const int scoreLayout = wantAdditive ? 1 : 0;

	// Switch widget visibility/layout ONLY on transition. Doing the hide/show
	// on every UpdateView caused a one-frame relayout strip on the secondary
	// screen on each score (Bug 1), and left the IJF labels mis-centred after
	// switching back to a classic ruleset (Bug 2). Steady state only sets text.
	if (m_scoreLayout != scoreLayout)
	{
		m_scoreLayout = scoreLayout;

		if (wantAdditive)
		{
			// the blink timer drives update_ippon() — must not fight the total
			if (m_pBlinkTimer && m_pBlinkTimer->isActive())
				m_pBlinkTimer->stop();

			set_ijf_lamps_visible(false);
			ui->text_wazaari_desc1->SetText("");
			ui->text_wazaari_desc2->SetText("");
			ui->text_yuko_desc1->SetText("");
			ui->text_yuko_desc2->SetText("");
			ui->text_ippon_first->SetBlinking(false);
			ui->text_ippon_second->SetBlinking(false);
			ui->text_ippon_first->show();
			ui->text_ippon_second->show();
		}
		else
		{
			// restore the full IJF layout; texts are re-driven by update_* below
			set_ijf_lamps_visible(true);
			ui->text_ippon_desc1->SetText("I");
			ui->text_ippon_desc2->SetText("I");
		}
	}

	if (wantAdditive)
	{
		update_additive_total(FighterEnum::First);
		update_additive_total(FighterEnum::Second);
		// a direct Hansoku-make still decides — keep it visible
		update_hansokumake(FighterEnum::First);
		update_hansokumake(FighterEnum::Second);
	}
	else
	{
		// first score
		update_ippon(FighterEnum::First);
		update_wazaari(FighterEnum::First);
		update_yuko(FighterEnum::First);
		update_shido(FighterEnum::First);
		update_operator_total(FighterEnum::First);
		update_hansokumake(FighterEnum::First);

		// second score
		update_ippon(FighterEnum::Second);
		update_wazaari(FighterEnum::Second);
		update_yuko(FighterEnum::Second);
		update_shido(FighterEnum::Second);
		update_operator_total(FighterEnum::Second);
		update_hansokumake(FighterEnum::Second);
	}

	update_team_score();

	//
	// timers
	//
	ui->text_main_clock->SetText(
		m_pController->GetTimeText(eTimer_Main),
		ScaledText::eSize_full);

	const FighterEnum holder(m_pController->GetLastHolder());
    
    if (FighterEnum::Nobody == holder && is_secondary())
	{
		ui->layout_info->setStretchFactor(ui->layout_osaekomi, 0);
	}

	switch (m_pController->GetCurrentState())
	{
	case eState_TimerRunning:
		{
			ui->text_main_clock->SetColor(m_MainClockColorRunning, m_MainClockBgColor);
			update_hold_clock(holder, eHoldState_pause);
		}
		break;

	case eState_TimerStopped:
		{
			ui->text_main_clock->SetColor(m_MainClockColorStopped, m_MainClockBgColor);
			update_hold_clock(holder, eHoldState_pause);
		}
		break;

	case eState_Holding:
		{
			ui->text_main_clock->SetColor(m_MainClockColorRunning, m_MainClockBgColor);
			update_hold_clock(holder, eHoldState_on);

			if (is_secondary())
			{
				ui->layout_info->setStretchFactor(ui->layout_name_first, 3);
				ui->layout_info->setStretchFactor(ui->layout_osaekomi, 2);
				ui->layout_info->setStretchFactor(ui->layout_name_second, 3);
			}
		}
		break;

	default:
		break;
	}

	// time is up?
	//const FighterEnum lead( m_pController->GetLead() );
	//if ( eState_TimerEnded == m_pController->GetCurrentState() )
	//{
	//	// something to do here?
	//}


//#ifdef _DEBUG
//	QString text;
//	switch(m_pController->GetCurrentState())
//	{
//		case eState_Holding:
//			text = "HOLDING";
//			break;
//		case eState_TimerRunning:
//			text = "RUNNING";
//			break;
//		case eState_TimerStopped:
//			text = "STOPPED";
//			break;
//		default:
//			text="OTHER";
//	}
//	ui->text_lastname_first->SetText(text);
//	update();
//#endif

	// Note: with update() the area is scheduled for a redraw
	//       while repaint() does this immediately.
	repaint();
	// This is really important, or timer values are not redrawn!
}

//=========================================================
void View::SetInfoHeaderFont(const QFont& font)
//=========================================================
{
	m_InfoHeaderFont = font;

	ui->text_weight->SetFont(font);
	ui->text_mat->SetFont(font);

	if (m_Edition == EditionType::Team)
	{
		ui->text_score_team_first_label->SetFont(font);
		ui->text_score_team_second_label->SetFont(font);
	}
}

//=========================================================
void View::SetFighterNameFont(const QFont& font)
//=========================================================
{
	m_FighterNameFont = font;

	ui->text_lastname_second->setAlignment(Qt::AlignLeft);
	ui->text_firstname_second->setAlignment(Qt::AlignLeft);
	ui->text_lastname_first->setAlignment(Qt::AlignRight);
	ui->text_firstname_first->setAlignment(Qt::AlignRight);

	ui->text_lastname_second->SetFont(font);
	ui->text_firstname_second->SetFont(font);
	ui->text_lastname_first->SetFont(font);
	ui->text_firstname_first->SetFont(font);
}

//=========================================================
void View::SetDigitFont(const QFont& font)
//=========================================================
{
	m_DigitFont = font;

	ui->text_main_clock->SetFont(font);
	ui->text_ippon_first->SetFont(font);
	ui->text_wazaari_first->SetFont(font);
	ui->text_yuko_first->SetFont(font);
	ui->text_ippon_second->SetFont(font);
	ui->text_wazaari_second->SetFont(font);
	ui->text_yuko_second->SetFont(font);
	ui->text_hold_clock_first->SetFont(font);
	ui->text_hold_clock_second->SetFont(font);

	if (m_Edition == EditionType::Team)
	{

		ui->text_score_team_first->SetFont(font);
		ui->text_score_team_second->SetFont(font);
	}
}

//=========================================================
void View::SetInfoTextColor(const QColor& color, const QColor& bgColor)
//=========================================================
{
	m_InfoTextColor = color;
	m_InfoTextBgColor = bgColor;

	ui->text_mat->SetColor(color, bgColor);
	ui->text_weight->SetColor(color, bgColor);
}


//=========================================================
void View::SetTextColorFirst(const QColor& color, const QColor& bgColor)
//=========================================================
{
	m_TextColorFirst = color;
	m_TextBgColorFirst = bgColor;

	update_colors();
}

//=========================================================
void View::SetTextColorSecond(const QColor& color, const QColor& bgColor)
//=========================================================
{
	m_TextColorSecond = color;
	m_TextBgColorSecond = bgColor;

	update_colors();
}


//=========================================================
void View::SetMainClockColor(const QColor& running, const QColor& stopped)
//=========================================================
{
	m_MainClockColorRunning = running;
	m_MainClockColorStopped = stopped;

	// colors are set automatically depending on current state
}

//=========================================================
void View::Reset()
//=========================================================
{
	ui->text_hold_clock_first->SetColor(Qt::gray, Qt::black);
	ui->text_hold_clock_second->SetColor(Qt::gray, Qt::black);
	ui->image_sand_clock->SetBgColor(Qt::black);
}

//=========================================================
void View::SetShowInfoHeader(bool show)
//=========================================================
{
	m_showInfoHeader = show;

	UpdateView();
}

//=========================================================
void View::paintEvent(QPaintEvent* /*event*/)
//=========================================================
{
	Q_ASSERT(m_pController && "IBController not set!");
}

//=========================================================
void View::mousePressEvent(QMouseEvent* event)
//=========================================================
{
	Q_ASSERT(m_pController && "IBController not set!");
    
    FighterEnum whos(FighterEnum::Nobody);
	EAction action(eAction_NONE);
	const bool doRevoke = event->button() & Qt::RightButton;

    ScaledImage* imageChild = dynamic_cast<ScaledImage*>(childAt(event->pos()));

    if (!imageChild)
	{
        ScaledText* textChild = dynamic_cast<ScaledText*>(childAt(event->pos()));

        if (!textChild)
			return;

        if (textChild == ui->text_main_clock)
		{
			if (doRevoke)   // right click!
			{
				QMenu menu;
                QAction* item(nullptr);

				item = menu.addAction(tr("Set time"));
				connect(item, SIGNAL(triggered()), this, SLOT(setMainTimerValue_()));

				item = menu.addAction(tr("Reset"));
				connect(item, SIGNAL(triggered()), this, SLOT(resetMainTimerValue_()));

				menu.exec(QCursor::pos());

				return;
			}

			action = eAction_Hajime_Mate;
		}
        else if (textChild == ui->text_hold_clock_first)
		{
			if (doRevoke)   // right click!
			{
				action = eAction_ResetOsaeKomi;
			}
			else
			{
				whos = FighterEnum::First;

				if (eState_Holding == m_pController->GetCurrentState() &&
						GVF_(FighterEnum::First) != m_pController->GetLead())
				{
					action = eAction_SetOsaekomi;
				}
				else
				{
					action = eAction_OsaeKomi_Toketa;
				}
			}
		}
        else if (textChild == ui->text_hold_clock_second)
		{
			if (doRevoke)   // right click!
			{
				action = eAction_ResetOsaeKomi;
			}
			else
			{
				whos = FighterEnum::Second;

				if (eState_Holding == m_pController->GetCurrentState() &&
						GVF_(FighterEnum::Second) != m_pController->GetLead())
				{
					action = eAction_SetOsaekomi;
				}
				else
				{
					action = eAction_OsaeKomi_Toketa;
				}
			}
		}
        else if (textChild == ui->text_ippon_second)
		{
			whos = FighterEnum::Second;
			action = eAction_Ippon;
		}
        else if (textChild == ui->text_ippon_first)
		{
			whos = FighterEnum::First;
			action = eAction_Ippon;
		}
        else if (textChild == ui->text_wazaari_second)
		{
			whos = FighterEnum::Second;
			action = eAction_Wazaari;
		}
        else if (textChild == ui->text_wazaari_first)
		{
			whos = FighterEnum::First;
			action = eAction_Wazaari;
		}
        else if (textChild == ui->text_yuko_second)
		{
			whos = FighterEnum::Second;
			action = eAction_Yuko;
		}
        else if (textChild == ui->text_yuko_first)
		{
			whos = FighterEnum::First;
			action = eAction_Yuko;
		}
		// JVP additive: Shido is shown as a count digit instead of the lamps, so
		// the digit must be clickable just like the lamps were (left = +Shido,
		// right = revoke via doRevoke). Mirrors the image_shido* cases below.
        else if (textChild == ui->text_shido_second)
		{
			whos = FighterEnum::Second;
			action = eAction_Shido;
		}
        else if (textChild == ui->text_shido_first)
		{
			whos = FighterEnum::First;
			action = eAction_Shido;
		}

		//else
		//{
		//	Q_ASSERT(!"action not defined for pointer!");
		//}
	}
	else
	{
        if (imageChild == ui->image_shido1_second)
		{
			whos = FighterEnum::Second;
			action = eAction_Shido;
		}
        else if (imageChild == ui->image_shido3_second ||
                 imageChild == ui->image_shido2_second ||
                 imageChild == ui->image_shido1_second)
		{
			whos = FighterEnum::Second;
			action = eAction_Shido;
		}
        else if (imageChild == ui->image_shido3_first ||
                 imageChild == ui->image_shido2_first ||
                 imageChild == ui->image_shido1_first)
		{
			whos = FighterEnum::First;
			action = eAction_Shido;
		}
        else if (imageChild == ui->image_hansokumake_second)
		{
			whos = FighterEnum::Second;
			action = eAction_Hansokumake;
		}
        else if (imageChild == ui->image_hansokumake_first)
		{
			whos = FighterEnum::First;
			action = eAction_Hansokumake;
		}

		//else
		//{
		//	Q_ASSERT(!"action not defined for pointer!");
		//}
	}

	whos = GVF_(whos); // get correct fighter for display
	m_pController->DoAction(action, whos, doRevoke);
}

//=========================================================
void View::setOsaekomiFirst_()
//=========================================================
{
	m_pController->DoAction(eAction_SetOsaekomi, FighterEnum::First, false/*doRevoke*/);
}

//=========================================================
void View::setOsaekomiSecond_()
//=========================================================
{
	m_pController->DoAction(eAction_SetOsaekomi, FighterEnum::Second, false/*doRevoke*/);
}

//=========================================================
void View::resetMainTimerValue_()
//=========================================================
{
    m_pController->DoAction(eAction_ResetMainTimer, FighterEnum::Nobody, true/*doRevoke*/);
}

//=========================================================
void View::setMainTimerValue_()
//=========================================================
{
	// Note: this is implemented in the MainWindowBase as well!

	//if( m_pController->GetCurrentState() == eState_SonoMama ||
	//  m_pController->GetCurrentState() == eState_TimerStopped )
	{
		bool ok(false);
		this->setStyleSheet("");
		const QString time = QInputDialog::getText(
								 this,
								 tr("Set Value"),
								 tr("Set value to (m:ss):"),
								 QLineEdit::Normal,
								 m_pController->GetTimeText(eTimer_Main),
								 &ok);

		if (ok)
		{
			m_pController->SetTimerValue(eTimer_Main, time);
		}
	}
}

//=========================================================
void View::blink_()
//=========================================================
{
	m_drawIppon = !m_drawIppon;

	if (is_secondary())
	{
		update_ippon(FighterEnum::First);
		update_ippon(FighterEnum::Second);
	}
}

//=========================================================
void View::update_ippon(Ipponboard::FighterEnum who) const
//=========================================================
{
	auto digit_ippon = ui->text_ippon_first;
	auto digit_wazaari = ui->text_wazaari_first;
	auto digit_yuko = ui->text_yuko_first;
	auto wazaariLabel = ui->text_wazaari_desc1;
	auto yukoLabel = ui->text_yuko_desc1;
	auto scoreLayout = ui->layout_score_first;
	FighterEnum uke(FighterEnum::Second);

	if (uke == who)
	{
		scoreLayout = ui->layout_score_second;
		digit_ippon = ui->text_ippon_second;
		digit_wazaari = ui->text_wazaari_second;
		digit_yuko = ui->text_yuko_second;
		wazaariLabel = ui->text_wazaari_desc2;
		yukoLabel = ui->text_yuko_desc2;
		uke = FighterEnum::First;
	}

	const int score = m_pController->GetScore(GVF_(who), Point::Ippon);

	// JVP additive (>1 Ippon allowed) on the operator console: render Ippon as a
	// plain count digit — exactly like Waza-ari/Yuko — instead of the IJF
	// "IPPON" wordmark + blink (which assumes Ippon=1 ends the bout and renders
	// the rotated wordmark). The spectator screen keeps the wordmark/blink below.
	if (!is_secondary() && m_pController->GetRules()->GetMaxIpponCount() > 1)
	{
		digit_ippon->SetBlinking(false);
		digit_ippon->show();
		digit_wazaari->show();
		wazaariLabel->SetText("W");

		if (m_pController->GetRules()->IsOption_HasYuko())
		{
			digit_yuko->show();
			yukoLabel->SetText("Y");
		}

		digit_ippon->SetText(QString::number(score), ScaledText::eSize_full);
		return;
	}

	if (score != 0)
	{
		Q_ASSERT(m_pBlinkTimer);

		if (!m_pBlinkTimer->isActive())
			m_pBlinkTimer->start(750);

		if (is_secondary())
		{
			if (m_drawIppon)
			{
				digit_ippon->show();
				digit_wazaari->hide();
				wazaariLabel->SetText("");

				if (m_pController->GetRules()->IsOption_HasYuko())
				{
					digit_yuko->hide();
					yukoLabel->SetText("");
				}
			}
			else
			{
				digit_ippon->hide();
				digit_wazaari->show();
				wazaariLabel->SetText("Waza-ari");

				if (m_pController->GetRules()->IsOption_HasYuko())
				{
					digit_yuko->show();
					yukoLabel->SetText("Yuko");
				}
			}
		}
		else
		{
			digit_ippon->show();
			digit_wazaari->show();
			wazaariLabel->SetText("W");

			if (m_pController->GetRules()->IsOption_HasYuko())
			{
				digit_yuko->show();
				yukoLabel->SetText("Y");
			}
		}

		digit_ippon->SetText("IPPON", ScaledText::eSize_full, !is_secondary());
	}
	else
	{
		const int score_uke = m_pController->GetScore(GVF_(uke), Point::Ippon);

		if (m_pBlinkTimer->isActive() &&  0 == score_uke)
		{
			m_pBlinkTimer->stop();
		}

		digit_ippon->SetBlinking(false);

		if (!is_secondary())
		{
			digit_ippon->show();
			digit_wazaari->show();
			wazaariLabel->SetText("W");

			if (m_pController->GetRules()->IsOption_HasYuko())
			{
				digit_yuko->show();
				yukoLabel->SetText("Y");
			}

			digit_ippon->SetText("-", ScaledText::eSize_full);
		}
		else
		{
			digit_ippon->hide();
			digit_wazaari->show();
			wazaariLabel->SetText("Waza-ari");

			if (m_pController->GetRules()->IsOption_HasYuko())
			{
				digit_yuko->show();
				yukoLabel->SetText("Yuko");
			}

			digit_ippon->SetText("");
		}
	}
}

//=========================================================
void View::update_wazaari(Ipponboard::FighterEnum who) const
//=========================================================
{
	ScaledText* digit(ui->text_wazaari_first);

	if (FighterEnum::Second == who)
		digit = ui->text_wazaari_second;

	const int score = m_pController->GetScore(GVF_(who), Point::Wazaari);
	//digit->setDigitCount( score > 9 ? 2 : 1 );
	digit->SetText(QString::number(score), ScaledText::eSize_full);
}

//=========================================================
void View::update_yuko(Ipponboard::FighterEnum who) const
//=========================================================
{
	ScaledText* digit(ui->text_yuko_first);

	if (FighterEnum::Second == who)
		digit = ui->text_yuko_second;

	const int score = m_pController->GetScore(GVF_(who), Point::Yuko);
	digit->SetText(QString::number(score), ScaledText::eSize_full);
}

//=========================================================
void View::update_shido(Ipponboard::FighterEnum who) const
//=========================================================
{
    ScaledImage* pImage1(nullptr);
    ScaledImage* pImage2(nullptr);
    ScaledImage* pImage3(nullptr);

	if (FighterEnum::First == who)
	{
		pImage1 = ui->image_shido1_first;
		pImage2 = ui->image_shido2_first;
		pImage3 = ui->image_shido3_first;
	}
	else
	{
		pImage1 = ui->image_shido1_second;
		pImage2 = ui->image_shido2_second;
		pImage3 = ui->image_shido3_second;
	}

	const int score = m_pController->GetScore(GVF_(who), Point::Shido);

	// JVP additive (uncapped Shido, no Hansoku-make escalation) on the operator
	// console: the 3 IJF lamps cannot represent 4+ Shido, so render a plain count
	// digit instead — mirroring the Ippon counter in update_ippon(). The spectator
	// screen folds Shido into the 0..20 total (wantAdditive) and never gets here.
	ScaledText* const digit =
		(FighterEnum::First == who) ? ui->text_shido_first : ui->text_shido_second;

	if (!is_secondary() && m_pController->GetRules()->GetMaxShidoCount() > 3)
	{
		pImage1->hide();
		pImage2->hide();
		pImage3->hide();
		digit->show();
		digit->SetText(QString::number(score), ScaledText::eSize_full);
		return;
	}

	// classic IJF lamp rendering: drop the additive digit and restore the two
	// always-on lamps (a runtime ruleset switch may have hidden them); shido3
	// visibility stays governed by UpdateView's GetMaxShidoCount() check.
	digit->hide();
	pImage1->show();
	pImage2->show();

	const auto imageOn = ":res/images/on.png";
	const auto imageOff = ":res/images/off.png";
	const auto imageEmpty = ":res/images/off_empty.png";

	pImage3->UpdateImage(score >= 3 ? imageOn : eTypePrimary == m_Type ? imageOff : imageEmpty);
	pImage2->UpdateImage(score >= 2 ? imageOn : eTypePrimary == m_Type ? imageOff : imageEmpty);
	pImage1->UpdateImage(score >= 1 ? imageOn : eTypePrimary == m_Type ? imageOff : imageEmpty);
}

//=========================================================
void View::update_hansokumake(Ipponboard::FighterEnum who) const
//=========================================================
{
	ScaledImage* pImage(ui->image_hansokumake_first);

	if (FighterEnum::Second == who)
	{
		pImage = ui->image_hansokumake_second;
	}

	const int score_hansokumake = m_pController->GetScore(GVF_(who), Point::Hansokumake);
	const int score_shido = m_pController->GetScore(GVF_(who), Point::Shido);

	// score_shido - 1 == cap (not cap + 1 == score_shido): cap is INT32_MAX for the
	// JVP additive ruleset and cap + 1 would overflow (UB). Equivalent for capped IJF.
	if (score_hansokumake > 0 || score_shido - 1 == m_pController->GetRules()->GetMaxShidoCount())
	{
		pImage->UpdateImage(":res/images/on_hansokumake.png");
	}
	else
	{
		pImage->UpdateImage(eTypePrimary == m_Type ? ":res/images/off_hansokumake.png" : ":res/images/off_empty.png");
	}
}

//=========================================================
void View::set_ijf_lamps_visible(bool visible) const
//=========================================================
{
	QWidget* const lamps[] =
	{
		ui->text_wazaari_first, ui->text_wazaari_second,
		ui->text_yuko_first, ui->text_yuko_second,
		ui->image_shido1_first, ui->image_shido2_first, ui->image_shido3_first,
		ui->image_shido1_second, ui->image_shido2_second, ui->image_shido3_second
	};

	for (QWidget* w : lamps)
		w->setVisible(visible);

	// the additive Shido count digit is operator-console-only and driven entirely
	// by update_shido(); it is never part of the IJF lamp set, so keep it hidden
	// here (this also clears it off the secondary additive screen, which shows the
	// 0..20 total instead and never calls update_shido).
	ui->text_shido_first->hide();
	ui->text_shido_second->hide();

	// the operator-console additive total is driven by update_operator_total() and
	// is never part of the IJF lamp set nor the secondary additive screen; keep it
	// hidden here so a layout transition (either direction) clears it.
	ui->text_total_first->hide();
	ui->text_total_second->hide();
}

//=========================================================
void View::update_additive_total(Ipponboard::FighterEnum who) const
//=========================================================
{
	// Steady state only: the widget visibility was already set on the layout
	// transition in UpdateView(). Here we just refresh the text — no hide/show,
	// so there is no per-score relayout on the secondary screen.
	ScaledText* digitTotal = ui->text_ippon_first;
	auto totalLabel = ui->text_ippon_desc1;

	if (FighterEnum::Second == who)
	{
		digitTotal = ui->text_ippon_second;
		totalLabel = ui->text_ippon_desc2;
	}

	const int total = m_pController->GetDisplayScore(GVF_(who));
	digitTotal->SetText(QString::number(total < 0 ? 0 : total), ScaledText::eSize_full);

	// Hiki-wake: regular time fully elapsed with no winner (no golden score in JVP)
	const bool drawnAtEnd = (m_pController->GetSecondsRemaining() == 0)
							 && (m_pController->GetWinner() == FighterEnum::Nobody);
	totalLabel->SetText(drawnAtEnd ? "HIKI-WAKE" : "");
}

//=========================================================
void View::update_operator_total(Ipponboard::FighterEnum who) const
//=========================================================
{
	// JVP additive scoring on the operator console: show each fighter's running
	// 0..20 total next to the main clock so the operator can track the score
	// (left = Second, right = First, matching the name/hold-clock layout). The
	// spectator screen folds the total into the big digit via update_additive_total()
	// and never gets here; mirrors the additive Shido digit in update_shido().
	ScaledText* const digit =
		(FighterEnum::First == who) ? ui->text_total_first : ui->text_total_second;

	if (!is_secondary() && m_pController->GetRules()->GetMaxShidoCount() > 3)
	{
		const int total = m_pController->GetDisplayScore(GVF_(who));
		digit->show();
		digit->SetText(QString::number(total < 0 ? 0 : total), ScaledText::eSize_full);
	}
	else
	{
		digit->hide();
	}
}

//=========================================================
void View::update_team_score() const
//=========================================================
{
	if (m_Edition == EditionType::Team)
	{
		if (is_secondary())
		{
			ui->text_score_team_first_label->SetText(m_pController->GetHomeLabel());
			ui->text_score_team_second_label->SetText(m_pController->GetGuestLabel());
		}
		else
		{
			ui->text_score_team_second_label->SetText(m_pController->GetHomeLabel());
			ui->text_score_team_first_label->SetText(m_pController->GetGuestLabel());
		}

		ui->text_score_team_first->SetText(
			QString::number(m_pController->GetTeamScore(GVF_(FighterEnum::First))),
			ScaledText::eSize_full);

		ui->text_score_team_second->SetText(
			QString::number(m_pController->GetTeamScore(GVF_(FighterEnum::Second))),
			ScaledText::eSize_full);
	}
	else
	{
		ui->text_score_team_first_label->SetText("");
		ui->text_score_team_second_label->SetText("");
		ui->text_score_team_first->SetText("");
		ui->text_score_team_second->SetText("");
	}
}

//=========================================================
void View::update_hold_clock(FighterEnum holder, EHoldState state) const
//=========================================================
{
	const QString value(m_pController->GetTimeText(eTimer_Hold));
	const FighterEnum first = GVF_(FighterEnum::First);
	const FighterEnum second = GVF_(FighterEnum::Second);

	struct ColorPair
	{
		ColorPair(QColor f = Qt::lightGray, QColor b = Qt::black)
			: fg(f), bg(b)
		{}

		QColor fg;
		QColor bg;
	};

	ColorPair hold_clock_colors[3][2];
	hold_clock_colors[eHoldState_on][FighterEnum::First] = ColorPair(m_TextColorFirst, m_TextBgColorFirst);
	hold_clock_colors[eHoldState_on][FighterEnum::Second] = ColorPair(m_TextColorSecond, m_TextBgColorSecond);
	hold_clock_colors[eHoldState_off][FighterEnum::First] = ColorPair(Qt::gray, Qt::black);
	hold_clock_colors[eHoldState_off][FighterEnum::Second] = ColorPair(Qt::gray, Qt::black);
	hold_clock_colors[eHoldState_pause][FighterEnum::First] = ColorPair(Qt::lightGray, m_TextBgColorFirst);
	hold_clock_colors[eHoldState_pause][FighterEnum::Second] = ColorPair(Qt::darkGray, m_TextBgColorSecond);

	ScaledText* pClocks[2] =
	{
		ui->text_hold_clock_first,
		ui->text_hold_clock_second
	};

	// reset drawing first
	pClocks[first]->SetColor(
		hold_clock_colors[eHoldState_off][FighterEnum::First].fg,
		hold_clock_colors[eHoldState_off][FighterEnum::First].bg);

	pClocks[second]->SetColor(
		hold_clock_colors[eHoldState_off][FighterEnum::Second].fg,
		hold_clock_colors[eHoldState_off][FighterEnum::Second].bg);

	pClocks[first]->SetText("00", ScaledText::eSize_full);
	pClocks[second]->SetText("00", ScaledText::eSize_full);
	ui->image_sand_clock->SetBgColor(Qt::black);

	// no one holding?
    if (FighterEnum::Nobody == holder)
	{
		if (is_secondary())
		{
			pClocks[FighterEnum::First]->SetText("", ScaledText::eSize_full);
			pClocks[FighterEnum::Second]->SetText("", ScaledText::eSize_full);
			ui->image_sand_clock->UpdateImage(":res/images/off_empty.png");
		}

		return;
	}

	// set spectial hold states
	if (eHoldState_on == state)
	{
		pClocks[GVF_(holder)]->SetColor(
			hold_clock_colors[eHoldState_on][holder].fg,
			hold_clock_colors[eHoldState_on][holder].bg);

		pClocks[GVF_(holder)]->SetText(value, ScaledText::eSize_full);

		ui->image_sand_clock->SetBgColor(
			hold_clock_colors[eHoldState_on][holder].bg);
	}
	else if (eHoldState_pause == state)
	{
		pClocks[GVF_(holder)]->SetColor(
			hold_clock_colors[eHoldState_pause][holder].fg,
			hold_clock_colors[eHoldState_pause][holder].bg);

		pClocks[GVF_(holder)]->SetText(value, ScaledText::eSize_full);

		ui->image_sand_clock->SetBgColor(
			hold_clock_colors[eHoldState_on][holder].bg);
	}
	else
	{
		Q_ASSERT(!"unknown state");
		return;
	}

	// on secondary screen only holder is shown
	if (is_secondary())
	{
		ui->image_sand_clock->UpdateImage(":res/images/sand_clock.png");
//		ui->layout_info->setStretchFactor(ui->layout_name_first, 4);
//		ui->layout_info->setStretchFactor(ui->layout_name_second, 4);

		if (FighterEnum::First == holder)
		{
			ui->layout_osaekomi->setStretchFactor(ui->text_hold_clock_first, 7);
			ui->layout_osaekomi->setStretchFactor(ui->text_hold_clock_second, 0);
		}
		else if (FighterEnum::Second == holder)
		{
			ui->layout_osaekomi->setStretchFactor(ui->text_hold_clock_first, 0);
			ui->layout_osaekomi->setStretchFactor(ui->text_hold_clock_second, 7);
		}
		else
		{
			Q_ASSERT(false);
		}
	}
}

//=========================================================
Ipponboard::FighterEnum View::GVF_(const Ipponboard::FighterEnum f) const
//=========================================================
{
	if (!is_secondary())
	{
		return (f == FighterEnum::First) ? FighterEnum::Second : FighterEnum::First;
	}

	return f;
}

//=========================================================
bool View::is_secondary() const
//=========================================================
{
	return eTypeSecondary == m_Type;
}

//=========================================================
const QColor& View::get_color(const ColorType t) const
//=========================================================
{
	const bool doSwap(eTypePrimary == m_Type);

	switch (t)
	{
	case firstFg:
		return doSwap ? m_TextColorSecond : m_TextColorFirst;

	case firstBg:
		return doSwap ? m_TextBgColorSecond : m_TextBgColorFirst;

	case secondFg:
		return doSwap ? m_TextColorFirst : m_TextColorSecond;

	case secondBg:
		return doSwap ? m_TextBgColorFirst : m_TextBgColorSecond;

	default:
		Q_ASSERT(!"unhandled switch case!");
	}

	return m_TextBgColorFirst; // just for debug purpose...
}

//=========================================================
void View::update_colors()
//=========================================================
{
	ui->text_score_team_first_label->SetColor(get_color(firstFg), get_color(firstBg));
	ui->text_score_team_first->SetColor(get_color(firstFg), get_color(firstBg));
	ui->dummy_first->SetBgColor(get_color(firstBg));
	ui->image_hansokumake_first->SetBgColor(get_color(firstBg));
	ui->image_shido3_first->SetBgColor(get_color(firstBg));
	ui->image_shido2_first->SetBgColor(get_color(firstBg));
	ui->image_shido1_first->SetBgColor(get_color(firstBg));
	ui->text_shido_first->SetColor(get_color(firstFg), get_color(firstBg));
	ui->text_total_first->SetColor(get_color(firstFg), get_color(firstBg));
	ui->text_yuko_first->SetColor(get_color(firstFg), get_color(firstBg));
	ui->text_wazaari_first->SetColor(get_color(firstFg), get_color(firstBg));
	ui->text_ippon_first->SetColor(get_color(firstFg), get_color(firstBg));
	ui->text_ippon_desc1->SetColor(get_color(firstFg), get_color(firstBg));
	ui->text_wazaari_desc1->SetColor(get_color(firstFg), get_color(firstBg));
	ui->text_yuko_desc1->SetColor(get_color(firstFg), get_color(firstBg));
	ui->text_lastname_first->SetColor(get_color(firstFg), get_color(firstBg));
	ui->text_firstname_first->SetColor(get_color(firstFg), get_color(firstBg));

	ui->text_score_team_second_label->SetColor(get_color(secondFg), get_color(secondBg));
	ui->text_score_team_second->SetColor(get_color(secondFg), get_color(secondBg));
	ui->dummy_second->SetBgColor(get_color(secondBg));
	ui->image_hansokumake_second->SetBgColor(get_color(secondBg));
	ui->image_shido3_second->SetBgColor(get_color(secondBg));
	ui->image_shido2_second->SetBgColor(get_color(secondBg));
	ui->image_shido1_second->SetBgColor(get_color(secondBg));
	ui->text_shido_second->SetColor(get_color(secondFg), get_color(secondBg));
	ui->text_total_second->SetColor(get_color(secondFg), get_color(secondBg));
	ui->text_yuko_second->SetColor(get_color(secondFg), get_color(secondBg));
	ui->text_wazaari_second->SetColor(get_color(secondFg), get_color(secondBg));
	ui->text_ippon_second->SetColor(get_color(secondFg), get_color(secondBg));
	ui->text_ippon_desc2->SetColor(get_color(secondFg), get_color(secondBg));
	ui->text_wazaari_desc2->SetColor(get_color(secondFg), get_color(secondBg));
	ui->text_yuko_desc2->SetColor(get_color(secondFg), get_color(secondBg));
	ui->text_lastname_second->SetColor(get_color(secondFg), get_color(secondBg));
	ui->text_firstname_second->SetColor(get_color(secondFg), get_color(secondBg));
}

//=========================================================
void View::show_golden_score(bool show)
//=========================================================
{
	ScaledImage* pWidgetGS(ui->image_golden_score);

	if (show)
		ui->horizontalLayout_bottom->setStretchFactor(pWidgetGS, 1);
	else
		ui->horizontalLayout_bottom->setStretchFactor(pWidgetGS, 0);
}
