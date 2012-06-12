#include <QPainter>
#include <QVBoxLayout>
#include <QScrollBar>
#include <QPainter>
#include <QWheelEvent>
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

			m_gradientPixmap = new QPixmap(1, header_height);

			QPainter p;
			p.begin(m_gradientPixmap);
			{
				QLinearGradient lg(0, 0, 0, 1);
				lg.setColorAt(0, QColor(204,204,204));
				lg.setColorAt(0.25, QColor(255,255,255));
				lg.setColorAt(1, QColor(128,128,128));
				lg.setCoordinateMode(QGradient::ObjectBoundingMode);

				p.setPen(Qt::NoPen);
				p.setBrush(lg);
				p.drawRect(0, 0, 1, header_height);
			}
			p.end();
		}
		~PatternView_Header()
		{
			delete m_gradientPixmap;
		}

		void paintEvent(QPaintEvent *)
		{
			QPainter p;
			p.begin(this);

			p.drawPixmap(0,0,width(), height(), *m_gradientPixmap);

			p.end();
		}

		QPixmap *m_gradientPixmap;
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
			: font("monospace"),
			  m_currentRowHighlightPixmap(NULL),
			  m_currentRowNoFocusHighlightPixmap(NULL),
			  m_currentRowRecordHighlightPixmap(NULL),
			  m_primaryHighlightPixmap(NULL),
			  m_secondaryHighlightPixmap(NULL),
			  m_modified(true)
		{
			font.setPixelSize(11);
			font.setBold(true);
			px_unit = font.pixelSize();
			px_hspace = px_unit * horizontal_factor;
			px_vspace = px_unit * vertical_factor;
			colspace = px_unit/2;
			opt.setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);

			redrawHighlightPixmaps();
		}
		~PatternView_Body()
		{
			if (m_currentRowHighlightPixmap != NULL)
				delete m_currentRowHighlightPixmap;
			if (m_currentRowNoFocusHighlightPixmap != NULL)
				delete m_currentRowNoFocusHighlightPixmap;
			if (m_currentRowRecordHighlightPixmap != NULL)
				delete m_currentRowRecordHighlightPixmap;
			if (m_primaryHighlightPixmap != NULL)
				delete m_primaryHighlightPixmap;
			if (m_secondaryHighlightPixmap != NULL)
				delete m_secondaryHighlightPixmap;
		}

		void createHighlightPixmap(QPixmap **pix, double r, double g, double b)
		{
			if (*pix != NULL)
				delete *pix;

			*pix = new QPixmap(1, px_vspace);

			QPainter p;
			p.begin(*pix);
			{
				const double x = 192;
				const double y = 128;
				const double z = 64;
				QLinearGradient lg(0, 0, 0, 1);
				lg.setColorAt(0, QColor(r*x, g*x, b*x));
				lg.setColorAt(0.25, QColor(r*y, g*y, b*y));
				lg.setColorAt(1, QColor(r*z, g*z, b*z));
				lg.setCoordinateMode(QGradient::ObjectBoundingMode);

				p.setPen(Qt::NoPen);
				p.setBrush(lg);
				p.drawRect(0, 0, 1, px_vspace);
			}
			p.end();
		}

		void redrawHighlightPixmaps()
		{
			createHighlightPixmap(&m_currentRowHighlightPixmap, 0,0,1);
			createHighlightPixmap(&m_currentRowNoFocusHighlightPixmap, 0.375,0.375,0.3);
			createHighlightPixmap(&m_currentRowRecordHighlightPixmap, 0.5625,0.18,0.25);
			createHighlightPixmap(&m_primaryHighlightPixmap, 0,0.25,0);
			createHighlightPixmap(&m_secondaryHighlightPixmap, 0,0.125,0);
		}

		PatternView * pvParent() const{ return m_pvParent; }

		PatternView * m_pvParent;

		// c=' ': -
		void drawChar(QPainter &p, int x, int y, char c, const QColor &col, bool selected)
		{
			if (c == ' ')
			{
				c = '-';
				p.setPen(selected?QColor(32, 64, 32):QColor(16, 32, 16));
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

		int yOffset() const
		{
			return height()/2 - px_vspace/2;
		}

		void paintEvent(QPaintEvent *)
		{
			const DocInfo *dinfo = gui::activeDocInfo();
			const FtmDocument *d = dinfo->doc();

			unsigned int frame = dinfo->currentFrame();
			unsigned int channels = d->GetAvailableChannels();

			QPainter p;
			p.begin(this);
			p.setPen(Qt::NoPen);
			p.setBrush(Qt::black);
			p.drawRect(rect());

			p.setFont(font);

			unsigned int row = dinfo->currentRow();

			int rowWidth = 0;
			for (int i = 0; i < channels; i++)
			{
				rowWidth += columnWidth(d->GetEffColumns(i)) + colspace;
			}

			const int row_x = px_unit*3 - colspace/2;

			const int y_offset = yOffset();
			const int frame_y_offset = y_offset - row*px_vspace;

			p.translate(0, frame_y_offset);

			// current row highlight
			QPixmap *highlight;
			if (pvParent()->hasFocus())
			{
				if (gui::isEditing())
				{
					highlight = m_currentRowRecordHighlightPixmap;
				}
				else
				{
					highlight = m_currentRowHighlightPixmap;
				}
			}
			else
			{
				highlight = m_currentRowNoFocusHighlightPixmap;
			}
			p.drawPixmap(row_x, row*px_vspace, rowWidth, px_vspace, *highlight);

			unsigned int currentframe_playlength = dinfo->framePlayLength(frame);

			int from = row - y_offset / px_vspace;
			int to = row + y_offset / px_vspace;

			int highlight_to = to+1>currentframe_playlength?currentframe_playlength:to+1;

			for (int i = from<0?0:from; i < highlight_to; i++)
			{
				if (i == row)
					continue;

				if (i % d->GetSecondHighlight() == 0)
				{
					// primary
					p.drawPixmap(row_x, i*px_vspace, rowWidth, px_vspace, *m_primaryHighlightPixmap);
				}
				else if (i % d->GetHighlight() == 0)
				{
					// secondary
					p.drawPixmap(row_x, i*px_vspace, rowWidth, px_vspace, *m_secondaryHighlightPixmap);
				}
			}

			if (from < 0)
			{
				// draw the previous frame
				if (frame > 0)
				{
					int prevlen = dinfo->framePlayLength(frame-1);
					int prevlen_pix = prevlen*px_vspace;

					p.translate(0, -prevlen_pix);

					drawFrame(p, frame-1, from+prevlen, prevlen-1, false);

					p.translate(0, prevlen_pix);
				}
			}

			int lr = drawFrame(p, frame, from, to, true);

			if (to > lr)
			{
				if (frame+1 < d->GetFrameCount())
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

			m_modified = false;
		}
		void mouseReleaseEvent(QMouseEvent *e)
		{
			if (gui::isPlaying())
				return;

			int y = (e->y() - yOffset())/px_vspace;
			if (e->y() - yOffset() < 0)
				y--;

			DocInfo *dinfo = gui::activeDocInfo();

			y += dinfo->currentRow();

			unsigned int frame = dinfo->currentFrame();

			if (y < 0)
			{
				if (frame == 0)
					return;
				frame--;
				y += dinfo->framePlayLength(frame);
				if (y < 0)
					return;
			}
			else if (y >= dinfo->framePlayLength(frame))
			{
				frame++;
				if (frame == dinfo->doc()->GetFrameCount())
					return;
				y -= dinfo->framePlayLength(frame-1);
				if (y >= dinfo->framePlayLength(frame))
					return;
			}

			dinfo->setCurrentRow(y);
			dinfo->setCurrentFrame(frame);
			gui::updateFrameChannel();
		}

		void setModified()
		{
			m_modified = true;
		}

		QFont font;
		int m_currentFrameRows;
		QPixmap *m_currentRowHighlightPixmap;
		QPixmap *m_currentRowNoFocusHighlightPixmap;
		QPixmap *m_currentRowRecordHighlightPixmap;
		QPixmap *m_primaryHighlightPixmap;
		QPixmap *m_secondaryHighlightPixmap;
		bool m_modified;
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

	void PatternView::keyPressEvent(QKeyEvent *e)
	{
		int k = e->key();
		if (k == Qt::Key_Enter || k == Qt::Key_Return)
		{
			gui::toggleSong();
		}
		if (k == Qt::Key_Space)
		{
			gui::toggleEditMode();
		}
		if (gui::isPlaying())
			return;
		DocInfo *dinfo = gui::activeDocInfo();
		if (k == Qt::Key_Up)
		{
			dinfo->scrollFrameBy(-1);
			gui::updateFrameChannel();
		}
		if (k == Qt::Key_Down)
		{
			dinfo->scrollFrameBy(1);
			gui::updateFrameChannel();
		}
	}

	void PatternView::wheelEvent(QWheelEvent *e)
	{
		if (gui::isPlaying())
			return;

		DocInfo *dinfo = gui::activeDocInfo();
		bool down = e->delta() < 0;

		dinfo->scrollFrameBy(down ? 4 : -4);
		gui::updateFrameChannel();
	}
	void PatternView::focusInEvent(QFocusEvent *e)
	{
		m_body->repaint();
		QAbstractScrollArea::focusInEvent(e);
	}
	void PatternView::focusOutEvent(QFocusEvent *e)
	{
		m_body->repaint();
		QAbstractScrollArea::focusOutEvent(e);
	}
	void PatternView::scrollContentsBy(int dx, int dy)
	{
		if (gui::isPlaying())
			return;

		DocInfo *dinfo = gui::activeDocInfo();
		dinfo->setCurrentRow(verticalScrollBar()->value());
		gui::updateFrameChannel();
	}

	void PatternView::update(bool modified)
	{
		DocInfo *dinfo = gui::activeDocInfo();

		if (modified || m_currentFrame != dinfo->currentFrame())
		{
			m_body->setModified();
		}

		m_currentFrame = dinfo->currentFrame();
		m_currentRow = dinfo->currentRow();
		m_currentChannel = dinfo->currentChannel();

		verticalScrollBar()->blockSignals(true);
		verticalScrollBar()->setRange(0, dinfo->framePlayLength(m_currentFrame)-1);
		verticalScrollBar()->setValue(m_currentRow);
		verticalScrollBar()->blockSignals(false);

		m_body->repaint();
	}
}
