// Copyright 2018 Florian Muecke. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE.txt file.

#include "ScaledText.h"
#include <QtGui>
#include <QtDebug>
#include <algorithm>

ScaledText::ScaledText(QWidget* pParent)
	: QWidget(pParent)
	, m_Text()
	, m_TextColor(Qt::color1)  // normal foreground
	, m_BGColor(Qt::transparent)  // normal background (transparent)
	, m_Alignment(Qt::AlignCenter)
    , m_pLayout(nullptr)
	, m_timerId(-1)
	, m_timerDelay(0)
	, m_textSize(eSize_normal)
	, m_isVisible(true)
    , m_isRotated(false)
    , m_autoFit(false)
    , m_baseFont()
{
	m_baseFont = font();
	SetText("ScaledText");
}

void ScaledText::SetAutoFit(bool enabled)
{
	m_autoFit = enabled;
	if (m_autoFit)
	{
		m_baseFont = font();
		apply_auto_fit();
	}
	Redraw();
}

void ScaledText::resizeEvent(QResizeEvent* event)
{
	QWidget::resizeEvent(event);
	if (m_autoFit)
	{
		apply_auto_fit();
		Redraw();
	}
}

void ScaledText::apply_auto_fit()
{
	if (width() <= 0 || height() <= 0)
		return;

	const int wMax = std::max(8, width() - 4);
	const int hMax = std::max(8, height() - 4);
	const int minPx = 6;
	int lo = minPx;
	int hi = hMax;
	int best = minPx;
	QFont candidate = m_baseFont;
	while (lo <= hi)
	{
		int mid = (lo + hi) / 2;
		candidate.setPixelSize(mid);
		QFontMetrics fm(candidate);
		const int tw = m_Text.isEmpty() ? 0 : fm.horizontalAdvance(m_Text);
		const int th = fm.height();
		if (tw <= wMax && th <= hMax)
		{
			best = mid;
			lo = mid + 1;
		}
		else
		{
			hi = mid - 1;
		}
	}
	QFont fitted = m_baseFont;
	fitted.setPixelSize(best);
	QWidget::setFont(fitted);
	delete m_pLayout;
	m_pLayout = new QTextLayout(m_Text, fitted);
	m_pLayout->setTextOption(QTextOption(m_Alignment));
	m_pLayout->setCacheEnabled(true);
	m_pLayout->beginLayout();
	m_pLayout->createLine();
	m_pLayout->endLayout();
}

void ScaledText::SetText(const QString& text,
						 ETextSize size,
						 bool rotate)
{
	const QString newText = (eSize_uppercase == size) ? text.toUpper() : text;

	// No-op when nothing changed. The operator console re-runs SetText on every
	// view refresh (each clock second, each score click), but the digit widgets
	// (Shido count, additive total, names, ...) usually hold the same value.
	// Rebuilding the text layout + repainting every tick made the digit flash its
	// background (paintEvent fills m_BGColor — white for fighter 1) and the row
	// appear to jump. Resizes are handled independently in resizeEvent(); colour
	// and font changes Redraw() on their own, so skipping here is safe.
	if (newText == m_Text && size == m_textSize && rotate == m_isRotated)
		return;

	set_size(size);
	m_Text = newText;
    m_isRotated = rotate;

	if (m_autoFit)
		apply_auto_fit();
	else
		update_text_metrics();

	Redraw();
}

void ScaledText::SetFont(const QFont& font)
{
	m_baseFont = font;
	this->setFont(font);
	if (m_autoFit)
		apply_auto_fit();
	else
		update_text_metrics();

	Redraw();
}

void ScaledText::SetFontAndColor(const QFont& font, const QColor& textColor,
								 const QColor& bgColor)
{
	SetFont(font);
	SetColor(textColor, bgColor);
}


void ScaledText::SetColor(const QColor& textColor, const QColor& bgColor)
{
	m_TextColor = textColor;
	m_BGColor = bgColor;

	Redraw();
}

void ScaledText::SetColor(const QColor& textColor)
{
	SetColor(textColor, m_BGColor);
}

const QColor& ScaledText::GetColor() const
{
	return m_TextColor;
}

const QColor& ScaledText::GetBgColor() const
{
	return m_BGColor;
}

void ScaledText::set_size(ETextSize size)
{
	Q_ASSERT(size < eSize_MAX && size >= 0);
	m_textSize = size;
}

ScaledText::ETextSize ScaledText::GetSize() const
{
	return m_textSize;
}

void ScaledText::SetBlinking(bool blink, int delay)
{
	if (blink && -1 != m_timerId && delay == m_timerDelay)
		return;

	// kill old timer
	if (-1 != m_timerId)
		killTimer(m_timerId);

	m_timerId = -1;
	m_timerDelay = delay;
	m_isVisible = true;

	if (blink)
		m_timerId = startTimer(delay);
}

bool ScaledText::IsBlinking() const
{
	return (m_timerId != -1);
}

void ScaledText::Redraw()
{
	update();
}

void ScaledText::paintEvent(QPaintEvent* event)
{
	QPainter painter(this);
	painter.save();

	// erase background
	painter.fillRect(event->rect(), QBrush(m_BGColor));

	if (m_isVisible && m_Text.contains(QChar('\n')))
	{
		painter.setRenderHint(QPainter::TextAntialiasing);
		painter.setPen(m_TextColor);

		const QStringList lines = m_Text.split('\n');
		QFontMetrics fm(this->font());
		const int lineH = fm.height();
		const int totalH = lineH * lines.size();
		int maxW = 1;
		for (const QString& s : lines)
			maxW = std::max(maxW, fm.horizontalAdvance(s));

		const qreal zoom = std::min(
			qreal(std::max(1, width() - 4)) / qreal(maxW),
			qreal(std::max(1, height() - 4)) / qreal(totalH));

		painter.translate(width() / 2.0, height() / 2.0);
		painter.scale(zoom, zoom);
		qreal y = -totalH / 2.0;
		Qt::Alignment hAlign = Qt::AlignHCenter;
		if (m_Alignment == Qt::AlignLeft) hAlign = Qt::AlignLeft;
		else if (m_Alignment == Qt::AlignRight) hAlign = Qt::AlignRight;
		for (const QString& s : lines)
		{
			painter.drawText(QRectF(-maxW / 2.0, y, maxW, lineH), hAlign | Qt::AlignVCenter, s);
			y += lineH;
		}

		painter.restore();
		painter.end();
		return;
	}

	if (m_isVisible)
	{
        painter.setRenderHint(QPainter::TextAntialiasing);

        if (m_pLayout)
		{
			painter.setPen(m_TextColor);

			// move coordinate system to the center
			painter.translate(width() / 2.0, height() / 2.0);

			QTextLine line = m_pLayout->lineAt(0);
			line.setLeadingIncluded(false);

            const QRectF textRect = line.naturalTextRect();
            Q_ASSERT(textRect == m_pLayout->boundingRect());

            auto w = textRect.width();
            auto h = textRect.height();
            qreal adjust_y{0};

            if (!m_isRotated)
			{
                /*
                    |    <-- Ascent
                    |    <-- Ascent
                    |    <-- Ascent
                    |    <-- Ascent
                    |--- <-- Baseline
                    |    <-- Descent
                    |    <-- Descent

                    height = ascent + descent
                    example: height=19, ascent=15, descent=4
                */

                if (eSize_full == m_textSize || eSize_uppercase == m_textSize)
				{
                    //qDebug() << "rect.width:" << textRect.width() << " rect.height:" << textRect.height() << "descent:" << line.descent() << "ascent:" << line.ascent();

                    // remove descent space
                    h = line.ascent();     // h => 15

                    // make extra ascent space a little smaller (almost never used)
                    h -= line.descent() / 2.0;  // h => 13

                    // calc vertical adjustment
                    adjust_y -= line.descent() / line.ascent();  // 4/15 => 0,266
                }
				else
				{
                    adjust_y = line.descent() / line.ascent();
				}
            }

            // consider that italic text is a little wider than normal text
            const qreal italicFactor = this->fontInfo().italic() ? 1.03 : 1.0;
            qreal zoomFactor{0};

            if (m_isRotated)
            {
                painter.rotate(-60.0);
                zoomFactor = std::min<qreal>(width() / h, height() / italicFactor / w);
            }
            else
            {
                zoomFactor = std::min<qreal>(width() / italicFactor / w, height() / h);
            }

            QPointF center = textRect.center();

            if (Qt::AlignLeft == m_Alignment)
			{
                center.setX(width() / 2.0 / italicFactor / zoomFactor);
			}
			else if (Qt::AlignRight == m_Alignment)
			{
                center.setX(-width() / 2.0 / italicFactor / zoomFactor + center.x() * 2);
			}

            center.setY(center.y() + adjust_y);

            painter.scale(zoomFactor, zoomFactor);
			line.draw(&painter, -center);
		}
	}

	painter.restore();
	painter.end();
}

void ScaledText::timerEvent(QTimerEvent* /*event*/)
{
	m_isVisible = !m_isVisible;

	Redraw();
}

void ScaledText::update_text_metrics()
{
	const QFont& currentFont = this->font();

	delete m_pLayout;
	m_pLayout = new QTextLayout(m_Text, currentFont);
	m_pLayout->setTextOption(QTextOption(m_Alignment));
	m_pLayout->setCacheEnabled(true);
	m_pLayout->beginLayout();
	qreal y = 0;
	QTextLine line = m_pLayout->createLine();
	while (line.isValid())
	{
		line.setLineWidth(10000);
		line.setPosition(QPointF(0, y));
		y += line.height();
		line = m_pLayout->createLine();
	}
	m_pLayout->endLayout();

	Redraw();
}
