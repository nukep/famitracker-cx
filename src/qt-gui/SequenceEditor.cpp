#include <QPainter>
#include <QLineEdit>
#include <QGridLayout>
#include <QLabel>
#include <QSpinBox>
#include <QScrollBar>
#include <QMouseEvent>
#include <QDebug>
#include "SequenceEditor.hpp"
#include "GUI.hpp"
#include "famitracker-core/FtmDocument.hpp"
#include "famitracker-core/Sequence.h"
#include "famitracker-core/Instrument.h"

namespace gui
{
	static const int volume_min = 0;
	static const int volume_max = 15;

	static const int arpeggio_rel_min = -96;
	static const int arpeggio_rel_max = 96;
	static const int arpeggio_abs_min = 0;
	static const int arpeggio_abs_max = 96;
	static const int arpeggio_window = 21;

	static const int pitch_min = -128;
	static const int pitch_max = 127;

	static const int hipitch_min = -128;
	static const int hipitch_max = 127;

	static const int dutynoise_min = 0;
	static const int dutynoise_max = 3;

	static const int pulsewidth_min = 0;
	static const int pulsewidth_max = 7;

	static const char looptoken[] = "|";
	static const char releasetoken[] = "/";

	static const int bottom_margin = 20;

	class SequenceEditorGraphic : public QWidget
	{
		friend class SequenceEditor;
	public:
		SequenceEditor * parent() const{ return (SequenceEditor*)parentWidget(); }

		void paintEvent(QPaintEvent *)
		{
			SequenceEditor *se = parent();
			QPainter p;
			p.begin(this);

			p.setFont(se->m_font);

			if (se->m_seq != NULL)
			{
				switch (se->m_seqtype)
				{
				case SEQ_VOLUME:
				case SEQ_DUTYCYCLE:
					se->drawBarGraph(p, se->minValVisible(), se->maxValVisible());
					break;
				case SEQ_ARPEGGIO:
					se->drawArpGraph(p, se->minValVisible(), se->maxValVisible());
					break;
				case SEQ_PITCH:
				case SEQ_HIPITCH:
					se->drawPitchGraph(p, se->minValVisible(), se->maxValVisible());
					break;
				default:
					break;
				}
			}
			else
			{

			}

			p.end();
		}
		void mousePressEvent(QMouseEvent *e)
		{
			m_cursor_origin = e->pos();
			parent()->dragMouse(m_cursor_origin, e->pos(), (e->button() & Qt::RightButton) != 0);
		}

		void mouseMoveEvent(QMouseEvent *e)
		{
			parent()->dragMouse(m_cursor_origin, e->pos(), (e->button() & Qt::RightButton) != 0);
		}

		QPoint m_cursor_origin;
	};

	SequenceEditor::SequenceEditor(int inst_type)
		: m_seq(NULL), m_doc(NULL), m_lineedit(NULL), m_inst_type(inst_type)
	{
		m_graphic = new SequenceEditorGraphic;
		m_font.setPixelSize(12);
		QGridLayout *g = new QGridLayout;

		int columns = 5;

		g->addWidget(m_graphic, 0, 0, 1, columns-1);

		m_scrollbar_arpwindow = new QScrollBar(Qt::Vertical);

		g->addWidget(m_scrollbar_arpwindow, 0, columns-1);

		g->addWidget(new QLabel(tr("Size:")));

		m_spinbox_size = new QSpinBox;
		m_spinbox_size->setRange(0, MAX_SEQUENCE_ITEMS);
		g->addWidget(m_spinbox_size, 1, 1);

		m_label_duration = new QLabel;
		g->addWidget(m_label_duration, 1, 2);

		g->setRowStretch(0, 0);
		g->setRowStretch(1, 0);

		g->setColumnStretch(3, 10);
		g->setMargin(0);
		g->setSpacing(0);
		setLayout(g);

		QObject::connect(m_scrollbar_arpwindow, SIGNAL(valueChanged(int)), this, SLOT(scrollArpWindow()));
		QObject::connect(m_spinbox_size, SIGNAL(valueChanged(int)), this, SLOT(changeSeqSize()));
	}
	SequenceEditor::~SequenceEditor()
	{

	}

	static inline bool isWhitespace(char c)
	{
		return c == ' ';
	}

	static const char * skipToNonWhitespace(const char *s)
	{
		while (true)
		{
			bool w = isWhitespace(*s);
			if (!w)
				break;
			s++;
		}
		return s;
	}

	static const char * textToken(const char *s, char *buf, int bufn)
	{
		s = skipToNonWhitespace(s);

		while (true)
		{
			char c = *s;
			bool e = c == 0 || isWhitespace(c);
			if (e)
				break;

			if (bufn > 1)
			{
				*buf = c;
				buf++;
				bufn--;
			}

			s++;
		}
		if (bufn > 0)
			*buf = 0;
		return s;
	}

	void SequenceEditor::parseText(const char *str)
	{
		// Takes an MML like format and translates it into a sequence

		const int BUFN = 16;
		char buf[BUFN];

		m_doc->lock();

		m_seq->Clear();

		int idx = 0;

		while (true)
		{
			str = textToken(str, buf, BUFN);
			if (buf[0] == 0)
			{
				// signifies there's nothing left to parse
				break;
			}
			if (strcmp(buf, looptoken) == 0)
			{
				m_seq->SetLoopPoint(idx);
			}
			else if (strcmp(buf, releasetoken) == 0)
			{
				m_seq->SetReleasePoint(idx);
			}
			else
			{
				int val = atoi(buf);
				if (val < minVal())
					val = minVal();
				if (val > maxVal())
					val = maxVal();

				m_seq->SetItem(idx, val);

				idx++;
			}
		}
		m_seq->SetItemCount(idx);

		m_doc->unlock();

		updateSequence(true);
	}

	void SequenceEditor::sequenceToLineEdit()
	{
		// caller locks m_doc

		QString s;
		int sz = m_seq->GetItemCount();

		int loop = m_seq->GetLoopPoint();
		int release = m_seq->GetReleasePoint();
		for (int i = 0; i < sz; i++)
		{
			char buf[16];

			if (i == loop)
			{
				sprintf(buf, "%s ", looptoken);
				s.append(buf);
			}
			else if (i == release)
			{
				sprintf(buf, "%s ", releasetoken);
				s.append(buf);
			}

			sprintf(buf, i==sz-1?"%d":"%d ", m_seq->GetItem(i));

			s.append(buf);
		}

		m_lineedit->blockSignals(true);
		m_lineedit->setText(s);
		m_lineedit->setCursorPosition(0);
		m_lineedit->blockSignals(false);
	}

	int SequenceEditor::minVal() const
	{
		switch (m_seqtype)
		{
		case SEQ_VOLUME:
			return volume_min;
		case SEQ_ARPEGGIO:
			if (m_seq->GetSetting() == 0)
				return arpeggio_rel_min;
			else
				return arpeggio_abs_min;
		case SEQ_PITCH:
			return pitch_min;
		case SEQ_HIPITCH:
			return hipitch_min;
		case SEQ_DUTYCYCLE:
			if (m_inst_type == INST_VRC6)
				return pulsewidth_min;
			else
				return dutynoise_min;
		default:
			return 0;
		}
	}
	int SequenceEditor::maxVal() const
	{
		switch (m_seqtype)
		{
		case SEQ_VOLUME:
			return volume_max;
		case SEQ_ARPEGGIO:
			if (m_seq->GetSetting() == 0)
				return arpeggio_rel_max;
			else
				return arpeggio_abs_max;
		case SEQ_PITCH:
			return pitch_max;
		case SEQ_HIPITCH:
			return hipitch_max;
		case SEQ_DUTYCYCLE:
			if (m_inst_type == INST_VRC6)
				return pulsewidth_max;
			else
				return dutynoise_max;
		default:
			return 0;
		}
	}
	int SequenceEditor::minValVisible() const
	{
		return minVal();
	}
	int SequenceEditor::maxValVisible() const
	{
		return maxVal();
	}

	void SequenceEditor::solveMetric(metric_t &m, int items)
	{
		int ymin = minValVisible();
		int ymax = maxValVisible();
		bool pitch = m_seqtype == SEQ_PITCH || m_seqtype == SEQ_HIPITCH;
		m.x = 30;
		m.y = 10;
		m.w = m_graphic->width() - m.x;
		m.h = m_graphic->height() - m.y - bottom_margin;

		if (pitch)
		{
			// no step height
			m.step_height = 0;

			m.step_width = m.h / 4;
		}
		else
		{
			m.step_height = m.h / (ymax - ymin);

			int hdiff = m.h - m.step_height * (ymax - ymin);

			m.y += hdiff;
			m.h -= hdiff;

			m.step_width = m.step_height*4;
		}

		if (m.step_width * items > m.w)
		{
			m.step_width = m.w / items;
		}
	}

	void SequenceEditor::posToSeqTuple(const metric_t &m, const QPoint &p, int &x, int &y) const
	{
		x = (p.x() - m.x) / m.step_width;
		if (p.x() < m.x)
			x--;

		if (m.step_height > 0)
		{
			y = (m.h - (p.y() - m.y) + m.step_height/2) / m.step_height + minValVisible();
		}
		else
		{
			y = (m.h - (p.y() - m.y)) * (maxValVisible() - minValVisible()) / m.h  + minValVisible();
		}
	}
	static inline void posToXPoint(const SequenceEditor::metric_t &m, const QPoint &p, int &x)
	{
		x = (p.x() - m.x + m.step_width/2) / m.step_width;
		if (p.x()+m.step_width/2 < m.x)
			x--;
	}

	void SequenceEditor::dragMouse(QPoint origin, QPoint pos, bool right)
	{
		{
			FtmDocument_lock_guard lock(m_doc);

			metric_t m;
			solveMetric(m, m_seq->GetItemCount());

			if (origin.y() > m.y + m.h)
			{
				// loop/release points
				int x;
				posToXPoint(m, pos, x);
				// we just need x

				bool discard = (x < 0 || x >= m_seq->GetItemCount());
				if (discard)
					x = -1;

				bool release = right;
				if (release)
				{
					m_seq->SetReleasePoint(x);
				}
				else
				{
					m_seq->SetLoopPoint(x);
				}
			}
			else
			{
				if (right)
				{
					// TODO
				}
				else
				{
					int x, y;
					posToSeqTuple(m, pos, x, y);
					if (x < 0 || x >= m_seq->GetItemCount())
					{
						m_doc->unlock();
						return;
					}
					if (y < minVal())
						y = minVal();
					if (y > maxVal())
						y = maxVal();

					m_seq->SetItem(x, y);
				}
			}
		}

		updateSequence();
	}

	void SequenceEditor::drawBar(QPainter &p, int x, int y, int w, int h)
	{
		w -= 2;
		h -= 1;

		if (h < 0)
		{
			y += h;
			h = -h;
		}
		if (h < 2)
			h = 2;

		p.setBrush(QColor(255,255,255,192));
		p.setPen(Qt::NoPen);

		p.drawRect(x, y, w, h);
		p.setPen(QColor(255,255,255));
		p.drawLine(x, y, x, y+h);
		p.drawLine(x, y, x+w, y);
		p.setPen(QColor(160,160,160));
		p.drawLine(x, y+h, x+w, y+h);
		p.drawLine(x+w, y, x+w, y+h);
	}

	void SequenceEditor::drawLoopReleasePoint(QPainter &p, bool loop, int point, const metric_t &o)
	{
		if (point < 0)
			return;
		if (loop)
		{
			p.setBrush(QColor(12,133,133));
		}
		else
		{
			p.setBrush(QColor(133,12,133));
		}
		p.setPen(Qt::NoPen);

		int x = o.x + point * o.step_width;
		int y = o.y + o.h;
		QRect r(x, y+1, (o.w+o.x) - x, bottom_margin-1);
		p.drawRect(r);

		p.setPen(Qt::white);
		p.drawLine(r.x(), r.y(), r.x(), r.bottom());
		p.drawText(r.adjusted(m_font.pixelSize()/2, 0, 0, 0), tr(loop ? "Loop" : "Release"), QTextOption(Qt::AlignVCenter | Qt::AlignLeft));
	}

	void SequenceEditor::drawScale(QPainter &p, int ymin, int ymax, int items, int loop, int release, const metric_t &m)
	{
		p.setBrush(Qt::black);
		p.setPen(Qt::NoPen);
		p.drawRect(rect());

		if (m.step_height > 2)
		{
			int sh = std::min(m.step_height, 12);
			QLinearGradient grad(0,0,0,1);
			const int steps = 6;
			for (int i = 0; i <= steps; i++)
			{
				qreal x = qreal(i) / qreal(steps);
				qreal ix = x;
				grad.setColorAt(x, QColor(48,48,48, 255*ix*ix*ix*ix));
			}
			grad.setCoordinateMode(QGradient::ObjectBoundingMode);
			p.setBrush(grad);
			for (int i = 0; i <= ymax - ymin; i++)
			{
				int y = m.y + m.h - i * m.step_height - sh;
				p.drawRect(m.x, y, m.w, sh);
			}
		}
		p.setBrush(QColor(255,255,255,32));

		for (int i = 0; i < items; i++)
		{
			if (i % 2 == 1)
			{
				p.drawRect(m.step_width * i + m.x, m.y, m.step_width, m.h);
			}
		}

		p.setPen(QColor(92,92,92));

		p.drawLine(m.x, m.y, m.x, m.y+m.h);

		p.setPen(Qt::white);

		// top label
		p.drawText(QRect(0, m.y, m.x, m_font.pixelSize()), QString("%1").arg(ymax), QTextOption(Qt::AlignVCenter | Qt::AlignHCenter));
		// bottom label
		p.drawText(QRect(0, m.y+m.h-m_font.pixelSize(), m.x, m_font.pixelSize()), QString("%1").arg(ymin), QTextOption(Qt::AlignVCenter | Qt::AlignHCenter));

		p.setPen(Qt::NoPen);
		p.setBrush(QColor(48,48,48));
		p.drawRect(m.x, m.y+m.h+1, m.w, m_graphic->height()-(m.y+m.h));

		drawLoopReleasePoint(p, true, loop, m);
		drawLoopReleasePoint(p, false, release, m);
	}

	void SequenceEditor::drawBarGraph(QPainter &p, int ymin, int ymax)
	{
		metric_t m;

		int count = m_seq->GetItemCount();

		solveMetric(m, count);

		drawScale(p, ymin, ymax, count, m_seq->GetLoopPoint(), m_seq->GetReleasePoint(), m);

		for (int i = 0; i < count; i++)
		{
			int val = m_seq->GetItem(i);
			if (val < ymin || val > ymax)
				continue;

			int x = i * m.step_width + m.x;
			int bar_h = (val-ymin) * m.step_height;
			int y = m.h - bar_h + m.y;
			drawBar(p, x, y, m.step_width, bar_h);
		}
	}

	void SequenceEditor::drawArpGraph(QPainter &p, int ymin, int ymax)
	{
		metric_t m;

		int count = m_seq->GetItemCount();

		solveMetric(m, count);

		drawScale(p, ymin, ymax, count, m_seq->GetLoopPoint(), m_seq->GetReleasePoint(), m);

		for (int i = 0; i < count; i++)
		{
			int val = m_seq->GetItem(i);
			if (val < ymin || val > ymax)
				continue;

			int x = i * m.step_width + m.x;
			int bar_h = (val-ymin) * m.step_height;
			int y = m.h - bar_h + m.y;
			drawBar(p, x, y, m.step_width, m.step_height);
		}
	}

	void SequenceEditor::drawPitchGraph(QPainter &p, int ymin, int ymax)
	{
		metric_t m;

		int count = m_seq->GetItemCount();

		solveMetric(m, count);

		drawScale(p, ymin, ymax, count, m_seq->GetLoopPoint(), m_seq->GetReleasePoint(), m);

		for (int i = 0; i < count; i++)
		{
			int val = m_seq->GetItem(i);
			if (val < ymin || val > ymax)
				continue;

			int x = i * m.step_width + m.x;
			int y_c = m.h/2 + m.y;
			int y_e = m.y + m.h - (val-ymin)*m.h/(ymax-ymin);
			int y_t = std::min(y_c, y_e);
			int y_b = std::max(y_c, y_e);
			drawBar(p, x, y_t, m.step_width, y_b - y_t);
		}
	}


	void SequenceEditor::setSequence(FtmDocument *doc, CSequence *seq, int type)
	{
		m_doc = doc;
		m_seq = seq;
		m_seqtype = type;
		m_scrollbar_arpwindow->setVisible(m_seqtype == SEQ_ARPEGGIO);
		updateSequence();
	}
	void SequenceEditor::updateSequence(bool fromlineedit)
	{
		FtmDocument_lock_guard lock(m_doc);

		int dur = m_seq->GetItemCount() * 1000 / m_doc->GetFrameRate();
		QString str = QString(tr("%1 ms").arg(dur));
		m_label_duration->setText(str);
		m_graphic->repaint();
		m_spinbox_size->blockSignals(true);
		m_spinbox_size->setValue(m_seq->GetItemCount());
		m_spinbox_size->blockSignals(false);
		if (!fromlineedit)
		{
			sequenceToLineEdit();
		}
	}

	void SequenceEditor::setLineEdit(QLineEdit *w)
	{
		QObject::disconnect(this, 0);
		m_lineedit = w;
		QObject::connect(m_lineedit, SIGNAL(editingFinished()), this, SLOT(lineEditEnter()));
	}

	void SequenceEditor::lineEditEnter()
	{
		parseText(m_lineedit->text().toUtf8());
	}
	void SequenceEditor::changeSeqSize()
	{
		if (m_spinbox_size->value() == m_seq->GetItemCount())
			return;

		m_seq->SetItemCount(m_spinbox_size->value());
		updateSequence();
	}
	void SequenceEditor::scrollArpWindow()
	{
		m_graphic->repaint();
	}
}
