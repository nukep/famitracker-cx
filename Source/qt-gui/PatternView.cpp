#include <QPainter>
#include <QVBoxLayout>
#include <QScrollBar>
#include <QPainter>
#include <QDebug>
#include <stdio.h>
#include <string.h>
#include "PatternView.hpp"
#include "gui.hpp"
#include "../FtmDocument.hpp"

namespace gui
{
	const int header_height = 40;

	class PatternView_Header : public QWidget
	{
	public:
		PatternView_Header()
		{
			setFixedHeight(header_height);
		}

		void paintEvent(QPaintEvent *)
		{
			QPainter p;
			p.begin(this);

			p.setPen(Qt::NoPen);
			p.setBrush(Qt::red);

			p.drawRect(rect());

			p.end();
		}

	};

	static const float vertical_factor = 1.25f;
	static const float horizontal_factor = 2.00f;

//	C, Cs, D, Ds, E, F, Fs, G, Gs, A, As, B

	static const char noteletters[] = "CCDDEFFGGAAB";
	static const char notesharps[] =  " # #  # # # ";

	class PatternView_Body : public QWidget
	{
	public:
		int px_unit, px_hspace, px_vspace;
		int colspace;
		QTextOption opt;
		PatternView_Body()
			: font("monospace")
		{
			font.setPixelSize(11);
			font.setBold(true);
			px_unit = font.pixelSize();
			px_hspace = px_unit * horizontal_factor;
			px_vspace = px_unit * vertical_factor;
			colspace = px_unit/2;
			opt.setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
		}
		~PatternView_Body()
		{
		}

		PatternView * pvParent() const{ return m_pvParent; }

		PatternView * m_pvParent;

		// c=' ': -
		void drawChar(QPainter &p, int x, int y, char c, const QColor &col, bool selected)
		{
			if (c == ' ')
			{
				c = '-';
				p.setPen(selected?QColor(64, 128, 64):QColor(32, 64, 32));
			}
			else
			{
				p.setPen(col);
			}
			p.drawText(QRect(x, y, px_unit, px_vspace), QString(c), opt);
		}

		int columnWidth(int effColumns)
		{
			return px_unit*6 + colspace*2 + (px_unit*3+colspace)*(effColumns+1);
		}

		// returns true if note terminates frame
		bool drawNote(QPainter &p, int x, int y, const stChanNote &n, int effColumns, bool selected)
		{
			const QColor primary = selected ? Qt::green : Qt::darkGreen;
			const QColor volcol = selected ? QColor(128, 128, 255) : QColor(64, 64, 128);
			const QColor effcol = selected ? QColor(255, 128, 128) : QColor(128, 64, 64);
			bool terminate = false;

			char buf[6];

			if (n.Note == NONE)
			{
				for (int i=0;i<3;i++)
					drawChar(p, x + px_unit*i, y, ' ', Qt::black, selected);
				x += px_unit*3 + colspace;
			}
			else if (n.Note == RELEASE)
			{
				// two bars
				p.setPen(Qt::NoPen);
				p.setBrush(primary);
				int hw = px_unit*2;
				int hh = px_vspace/4;
				int hth = px_vspace*2/3;
				p.drawRect( x + (px_unit*3-hw)/2, y + (px_vspace-hth)/2, hw, hh);
				p.drawRect( x + (px_unit*3-hw)/2, y + hth-(px_vspace-hth)/2, hw, hh);

				x += px_unit*3 + colspace;
			}
			else if (n.Note == HALT)
			{
				// one bar
				p.setPen(Qt::NoPen);
				p.setBrush(primary);
				int hw = px_unit*2;
				int hh = px_vspace/4;
				p.drawRect( x + (px_unit*3-hw)/2, y + (px_vspace-hh)/2, hw, hh);
				x += px_unit*3 + colspace;
			}
			else
			{
				int ni = n.Note - C;
				sprintf(buf, "%c", noteletters[ni]);
				drawChar(p, x, y, buf[0], primary, selected);
				x += px_unit;

				buf[0] = notesharps[ni];
				if (buf[0] == ' ') buf[0] = '-';
				drawChar(p, x, y, buf[0], primary, selected);
				x += px_unit;

				sprintf(buf, "%d", n.Octave);
				drawChar(p, x, y, buf[0], primary, selected);
				x += px_unit + colspace;
			}

			if (n.Instrument >= MAX_INSTRUMENTS)
				sprintf(buf, "  ");
			else
				sprintf(buf, "%02X", n.Instrument);
			drawChar(p, x, y, buf[0], primary, selected);
			x += px_unit;
			drawChar(p, x, y, buf[1], primary, selected);
			x += px_unit + colspace;

			if (n.Vol > 0xF)
				buf[0] = ' ';
			else
				sprintf(buf, "%X", n.Vol);
			drawChar(p, x, y, buf[0], volcol, selected);
			x += px_unit + colspace;

			for (unsigned int i = 0; i <= effColumns; i++)
			{
				unsigned char eff = n.EffNumber[i];

				if (eff == EF_JUMP || eff == EF_SKIP || eff == EF_HALT)
					terminate = true;

				if (eff == 0)
					buf[0] = ' ';
				else
					buf[0] = EFF_CHAR[eff-1];
				drawChar(p, x, y, buf[0], effcol, selected);
				x += px_unit;

				if (eff == 0)
				{
					sprintf(buf, "  ");
				}
				else
				{
					unsigned char effp = n.EffParam[i];
					sprintf(buf, "%02X", effp);
				}
				drawChar(p, x, y, buf[0], primary, selected);
				x += px_unit;
				drawChar(p, x, y, buf[1], primary, selected);
				x += px_unit + colspace;
			}

			return terminate;
		}

		// returns last row drawn ("to")
		int drawFrame(QPainter &p, unsigned int frame, int from, int to, bool selected)
		{
			FtmDocument *d = gui::activeDocument();
			unsigned int patternLength = d->GetPatternLength();

			unsigned int track = d->GetSelectedTrack();

			unsigned int channels = d->GetAvailableChannels();

			from = from < 0 ? 0 : from;
			to = to > patternLength-1 ? patternLength-1 : to;

			for (int i = from; i <= to; i++)
			{
				int y = px_vspace*i;

				char buf[6];
				p.setPen(selected?Qt::green:Qt::darkGreen);
				sprintf(buf, "%02X", i);

				p.drawText(QRect(0,y,px_unit*3-colspace/2,px_vspace), buf, opt);

				int x = px_unit*3;

				bool terminateFrame = false;

				for (unsigned int j = 0; j < channels; j++)
				{
					stChanNote note;

					d->GetDataAtPattern(track, d->GetPatternAtFrame(frame, j), j, i, &note);

					unsigned int effcolumns = d->GetEffColumns(j);

					terminateFrame |= drawNote(p, x, y, note, effcolumns, selected);

					x += columnWidth(effcolumns) + colspace;
				}

				if (terminateFrame)
				{
					return i;
				}
			}
			return to;
		}

		void paintEvent(QPaintEvent *)
		{
			FtmDocument *d = gui::activeDocument();

			unsigned int channels = d->GetAvailableChannels();

			QPainter p;
			p.begin(this);
			p.setPen(Qt::NoPen);
			p.setBrush(Qt::black);
			p.drawRect(rect());

			p.setFont(font);

			unsigned int row = gui::activeDocInfo()->currentRow();

			int rowWidth = 0;
			for (int i = 0; i < channels; i++)
			{
				rowWidth += columnWidth(d->GetEffColumns(i)) + colspace;
			}

			const int y_offset = height()/2 - px_vspace/2;
			const int frame_y_offset = y_offset - row*px_vspace;

			p.translate(0, frame_y_offset);

			{
				QLinearGradient lg(0, 0, 0, 1);
				lg.setColorAt(0, QColor(0, 0, 128));
				lg.setColorAt(1, QColor(0, 0, 64));
				lg.setCoordinateMode(QGradient::ObjectBoundingMode);

				p.setBrush(lg);
				p.drawRect(px_unit*3 - colspace/2, row*px_vspace, rowWidth, px_vspace);
			}

			int from = row - y_offset / px_vspace;
			int to = row + y_offset / px_vspace;

			unsigned int frame = gui::activeDocInfo()->currentFrame();

			int lr = drawFrame(p, frame, from, to, true);

			if (to > lr)
			{
				if (frame+1 < gui::activeDocument()->GetFrameCount())
				{
					// draw the next frame, too
					p.translate(0, (lr+1)*px_vspace);
					drawFrame(p, frame+1, 0, to-lr-1, false);
				}
			}

			p.resetTransform();

			p.setPen(QColor(80,80,80));
			int x = px_unit*3 - colspace/2;
			for (int i = 0; i <= channels; i++)
			{
				p.drawLine(x, 0, x, height());
				x += columnWidth(d->GetEffColumns(i)) + colspace;
			}
			p.end();
		}

		QFont font;
		int m_currentFrameRows;
	};

	PatternView::PatternView(QWidget *parent)
		: QAbstractScrollArea(parent),
		  m_currentFrame(0), m_currentRow(0), m_currentChannel(0)
	{
		setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
		setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);

		m_header = new PatternView_Header;
		m_body = new PatternView_Body;
		m_body->m_pvParent = this;

		QVBoxLayout *l = new QVBoxLayout;
		l->setMargin(0);
		l->setSpacing(0);
		l->addWidget(m_header);
		l->addWidget(m_body);

		viewport()->setLayout(l);
	}

	void PatternView::update(bool modified)
	{
		DocInfo *dinfo = gui::activeDocInfo();
		FtmDocument *d = dinfo->doc();

		if (modified || m_currentFrame != dinfo->currentFrame())
		{
		//	m_body->repaintPixmaps();
		}

		m_currentFrame = dinfo->currentFrame();
		m_currentRow = dinfo->currentRow();
		m_currentChannel = dinfo->currentChannel();

		m_body->repaint();
	}
}
