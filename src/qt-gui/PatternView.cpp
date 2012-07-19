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
#include "famitracker-core/FtmDocument.hpp"
#include "famitracker-core/App.hpp"

namespace gui
{
	const int header_height = 40;

	static const float vertical_factor = 1.25f;
	static const float horizontal_factor = 2.00f;

//	C, Cs, D, Ds, E, F, Fs, G, Gs, A, As, B

	static const char noteletters[] = "CCDDEFFGGAAB";
	static const char notesharps[] =  " # #  # # # ";

	static bool scancodeToNote(int scancode, int octave_base, int &note, int &octave)
	{
		// TODO: mac version
		// the nice thing about scan codes is it doesn't matter what locale
		// your keyboard is.
		// TODO: other keyboard note styles

		// This maps to FastTracker 2/FL Studio style

		int ob = octave_base;

		const int b1 = 10;	// top black row		(1)
		const int w1 = 24;	// top white row		(Q)
		const int b2 = 38;	// bottom black row		(A)
		const int w2 = 52;	// bottom white row		(Z)

		switch (scancode)
		{
		// number row
		case b1+1: note = Cs; octave = ob+1; break;
		case b1+2: note = Ds; octave = ob+1; break;
		case b1+4: note = Fs; octave = ob+1; break;
		case b1+5: note = Gs; octave = ob+1; break;
		case b1+6: note = As; octave = ob+1; break;
		case b1+8: note = Cs; octave = ob+2; break;
		case b1+9: note = Ds; octave = ob+2; break;

		case w1+0: note = C; octave = ob+1; break;
		case w1+1: note = D; octave = ob+1; break;
		case w1+2: note = E; octave = ob+1; break;
		case w1+3: note = F; octave = ob+1; break;
		case w1+4: note = G; octave = ob+1; break;
		case w1+5: note = A; octave = ob+1; break;
		case w1+6: note = B; octave = ob+1; break;
		case w1+7: note = C; octave = ob+2; break;
		case w1+8: note = D; octave = ob+2; break;
		case w1+9: note = E; octave = ob+2; break;
		case w1+10: note = F; octave = ob+2; break;

		case b2+1: note = Cs; octave = ob; break;
		case b2+2: note = Ds; octave = ob; break;
		case b2+4: note = Fs; octave = ob; break;
		case b2+5: note = Gs; octave = ob; break;
		case b2+6: note = As; octave = ob; break;
		case b2+8: note = Cs; octave = ob+1; break;
		case b2+9: note = Ds; octave = ob+1; break;

		case w2+0: note = C; octave = ob; break;
		case w2+1: note = D; octave = ob; break;
		case w2+2: note = E; octave = ob; break;
		case w2+3: note = F; octave = ob; break;
		case w2+4: note = G; octave = ob; break;
		case w2+5: note = A; octave = ob; break;
		case w2+6: note = B; octave = ob; break;
		case w2+7: note = C; octave = ob+1; break;
		case w2+8: note = D; octave = ob+1; break;

		default:
			return false;
		}
		return true;
	}

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
			const QColor instcol = primary;
			const QColor noinstcol = selected ? Qt::red : Qt::darkRed;
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

			const QColor *use_instcol = &instcol;
			if (n.Instrument >= MAX_INSTRUMENTS)
			{
				sprintf(buf, "  ");
			}
			else
			{
				if (gui::activeDocument()->GetInstrument(n.Instrument) == NULL)
				{
					// instrument does not exist
					use_instcol = &noinstcol;
				}
				sprintf(buf, "%02X", n.Instrument);
			}
			drawChar(p, x, y, buf[0], *use_instcol, selected);
			x += px_unit;
			drawChar(p, x, y, buf[1], *use_instcol, selected);
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

		int patterncol_offset(unsigned int c) const
		{
			switch(c)
			{
			case C_NOTE: return 0;
			case C_INSTRUMENT1: return px_unit*3 + colspace;
			case C_INSTRUMENT2: return px_unit*4 + colspace;
			case C_VOLUME: return px_unit*5 + colspace*2;
		//	case C_EFF_NUM: return px_unit*6 + colspace*3;
			default:
			{
				int off = px_unit*6 + colspace*3;
				c = c - C_EFF_NUM;
				off += c*px_unit + (c/C_EFF_COL_COUNT)*colspace;
				return off;
			}
			}
		}
		int channel_width(int effColumns) const
		{
			return px_unit*6 + colspace*3 + (px_unit*C_EFF_COL_COUNT + colspace)*(effColumns+1);
		}

		void paintEvent(QPaintEvent *)
		{
			const DocInfo *dinfo = gui::activeDocInfo();
			FtmDocument *d = dinfo->doc();

			d->lock();

			unsigned int frame = dinfo->currentFrame();
			unsigned int channels = d->GetAvailableChannels();

			d->unlock();

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

			int chancol_hwidth = 1;
			int cur_chancol = dinfo->currentChannelColumn();
			if (cur_chancol == 0)
				chancol_hwidth = 3;
			int chancol_x = px_unit*3 + patterncol_offset(cur_chancol);
			for (unsigned int i = 0; i < dinfo->currentChannel(); i++)
			{
				chancol_x += channel_width(d->GetEffColumns(i));
			}
			p.setPen(Qt::NoPen);
			p.setBrush(Qt::darkGray);
			p.drawRect(chancol_x, row*px_vspace, chancol_hwidth*px_unit, px_vspace);


			unsigned int currentframe_playlength = d->getFramePlayLength(frame);

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
					int prevlen = d->getFramePlayLength(frame-1);
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
			DocInfo *dinfo = gui::activeDocInfo();
			FtmDocument *d = dinfo->doc();

			int newchan, newcol;
			channelColAtX(e->x(), newchan, newcol);
			if (newchan >= 0)
			{
				dinfo->setCurrentChannel(newchan);
				dinfo->setCurrentChannelColumn(newcol);
			}

			if (!gui::isPlaying())
			{
				int y = (e->y() - yOffset())/px_vspace;
				if (e->y() - yOffset() < 0)
					y--;

				y += dinfo->currentRow();

				unsigned int frame = dinfo->currentFrame();

				if (y < 0)
				{
					if (frame == 0)
						goto norowframe;
					frame--;
					y += d->getFramePlayLength(frame);
					if (y < 0)
						goto norowframe;
				}
				else if (y >= d->getFramePlayLength(frame))
				{
					frame++;
					if (frame == dinfo->doc()->GetFrameCount())
						goto norowframe;
					y -= d->getFramePlayLength(frame-1);
					if (y >= d->getFramePlayLength(frame))
						goto norowframe;
				}

				dinfo->setCurrentRow(y);
				dinfo->setCurrentFrame(frame);
			}

		norowframe:
			gui::updateFrameChannel();
		}

		void setModified()
		{
			m_modified = true;
		}

		// return -1 if none
		int channelAtX(int x)
		{
			x = x - px_unit*3;
			if (x < 0)
				return -1;

			FtmDocument *d = gui::activeDocument();

			unsigned int channels = d->GetAvailableChannels();

			for (int i = 0; i < channels; i++)
			{
				int eff = d->GetEffColumns(i);
				int w = columnWidth(eff) + colspace;
				x -= w;
				if (x < 0)
					return i;
			}

			return -1;
		}
		int xAtChannel(int channel)
		{
			FtmDocument *d = gui::activeDocument();

			int x = px_unit*3 - colspace/2;
			for (int i = 0; i < channel; i++)
			{
				x += columnWidth(d->GetEffColumns(i)) + colspace;
			}
			return x;
		}
		int colAtRelX(int x)
		{
			int col = 0;
			while (true)
			{
				int off = patterncol_offset(col+1);

				if (x < off)
					return col;
				col++;
			}
		}
		void channelColAtX(int x, int &channel, int &col)
		{
			channel = channelAtX(x);
			if (channel < 0)
				return;
			int cx = xAtChannel(channel);
			col = colAtRelX(x - cx);
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

	class PatternView_Header : public QWidget
	{
	public:
		PatternView_Header(PatternView_Body *body)
			: m_body(body)
		{
			m_font.setPixelSize(12);
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

		void drawLine(QPainter &p, int x, int y, int h)
		{
			p.setPen(QColor(128,128,128));
			p.drawLine(x, y, x, y+h);
			x++;
			p.setPen(QColor(255,255,255));
			p.drawLine(x, y, x, y+h);
		}
		void drawHLine(QPainter &p, int x, int y, int w)
		{
			p.setPen(QColor(128,128,128));
			p.drawLine(x, y, x+w, y);
			y++;
			p.setPen(QColor(255,255,255));
			p.drawLine(x, y, x+w, y);
		}

		// vol is 0..15
		void drawVolume(QPainter &p, int x, int y, int vol)
		{
			int squaresize = 5;
			int stride = squaresize + 2;
			int nx, ny;

			p.setPen(Qt::NoPen);


			p.setBrush(Qt::darkGray);
			nx = x+1;
			ny = y+1;

			for (int i = 0; i < 15; i++)
			{
				p.drawRect(nx, ny, squaresize, squaresize);

				nx += stride;
			}

			p.setBrush(Qt::green);
			nx = x;
			ny = y;
			for (int i = 0; i < 15; i++)
			{
				if (i == vol)
				{
					p.setBrush(Qt::gray);

				}
				p.drawRect(nx, ny, squaresize, squaresize);

				nx += stride;
			}
		}

		void paintEvent(QPaintEvent *)
		{
			FtmDocument *d = gui::activeDocument();

			QPainter p;
			p.begin(this);

			p.drawPixmap(0,0,width(), height(), *m_gradientPixmap);

			drawHLine(p, 0,0,width());
			drawLine(p, 0,1, height()-1);

			int x = m_body->xAtChannel(0);
			drawLine(p, x, 1, height()-1);

			p.setFont(m_font);

			const core::u8 *vols = gui::activeDocInfo()->volumes();

			for (int i = 0; i < d->GetAvailableChannels(); i++)
			{
				bool enabled = !gui::isMuted(i);
				int w = m_body->xAtChannel(i+1) - x;
				p.setPen(enabled ? Qt::black : Qt::red);
				QRect r = QRect(x, 0, w, height()/2).adjusted(m_font.pixelSize(), 0,0,0);
				QTextOption opt(Qt::AlignBottom | Qt::AlignLeft);
				const char *name = app::channelMap()->GetChannelName(d->getChannelsFromChip()[i]);

				p.drawText(r, name, opt);
				drawVolume(p, x + 5, height()/2 + 5, vols[i]);
				x += w;
				drawLine(p, x, 1, height()-1);
			}

			p.end();
		}
		void mousePressEvent(QMouseEvent *e)
		{
			int chan = m_body->channelAtX(e->x());
			if (chan < 0)
				return;

			gui::toggleMuted(chan);
			repaint();
		}
		void mouseDoubleClickEvent(QMouseEvent *e)
		{
			int chan = m_body->channelAtX(e->x());
			if (chan < 0)
				return;

			gui::toggleSolo(chan);
			repaint();
		}

		QPixmap *m_gradientPixmap;
		QFont m_font;
		PatternView_Body *m_body;
	};

	PatternView::PatternView(QWidget *parent)
		: QAbstractScrollArea(parent),
		  m_currentFrame(0), m_currentRow(0), m_currentChannel(0)
	{
		setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
		setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);

		m_body = new PatternView_Body;
		m_header = new PatternView_Header(m_body);
		m_body->m_pvParent = this;

		QVBoxLayout *l = new QVBoxLayout;
		l->setMargin(0);
		l->setSpacing(0);
		l->addWidget(m_header);
		l->addWidget(m_body);

		viewport()->setLayout(l);
	}

	bool PatternView::event(QEvent *e)
	{
		if (e->type() == QEvent::KeyPress)
		{
			QKeyEvent *k = (QKeyEvent*)e;
			int key = k->key();
			if (key == Qt::Key_Backtab || key == Qt::Key_Tab)
			{
				// intercept tab/backtab, which usually take away focus
				keyPressEvent(k);
				return true;
			}
		}
		return QAbstractScrollArea::event(e);
	}
	void PatternView::keyPressEvent(QKeyEvent *e)
	{
		DocInfo *dinfo = gui::activeDocInfo();

		if (dinfo->currentChannelColumn() == C_NOTE)
		{
			int scan = e->nativeScanCode();
			int octave_base = 3;
			int note=-1, octave=-1;
			if (scancodeToNote(scan, octave_base, note, octave))
			{
				// ignore auto repeating notes
				if (e->isAutoRepeat())
					return;

				gui::auditionNote(dinfo->currentChannel(), octave, note);

				enterNote(note, octave);
				return;
			}
		}
		int k = e->key();
		if (k == Qt::Key_Backslash)
		{
			// release note
			enterNote(RELEASE, 0);

			return;
		}
		if (k == Qt::Key_Apostrophe)
		{
			// halt note
			// The original Windows FamiTracker fucks up the default, so we're using apostrophe for now
			enterNote(HALT, 0);

			return;
		}
		if (k == Qt::Key_Delete)
		{
			deleteColumn();
			return;
		}
		if (k == Qt::Key_Tab)
		{
			dinfo->scrollChannelBy(1);
			gui::updateFrameChannel();
			return;
		}
		if (k == Qt::Key_Backtab)
		{
			dinfo->scrollChannelBy(-1);
			gui::updateFrameChannel();
			return;
		}
		if (k == Qt::Key_Left)
		{
			dinfo->scrollChannelColumnBy(-1);
			gui::updateFrameChannel();
			return;
		}
		if (k == Qt::Key_Right)
		{
			dinfo->scrollChannelColumnBy(1);
			gui::updateFrameChannel();
			return;
		}
		if (k == Qt::Key_Enter || k == Qt::Key_Return)
		{
			gui::toggleSong();
			return;
		}
		if (k == Qt::Key_Space)
		{
			gui::toggleEditMode();
			return;
		}
		if (gui::isPlaying())
			return;
		if (k == Qt::Key_Up)
		{
			dinfo->scrollFrameBy(-dinfo->editStep());
			gui::updateFrameChannel();
			return;
		}
		if (k == Qt::Key_Down)
		{
			dinfo->scrollFrameBy(dinfo->editStep());
			gui::updateFrameChannel();
			return;
		}

		enterKeyAtColumn(k);
	}

	void PatternView::keyReleaseEvent(QKeyEvent *)
	{
		gui::auditionNoteHalt();
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
	void PatternView::deleteColumn()
	{
		if (!gui::canEdit())
			return;
		DocInfo *dinfo = gui::activeDocInfo();
		FtmDocument *doc = dinfo->doc();

		doc->lock();
		doc->DeleteNote(dinfo->currentFrame(), dinfo->currentChannel(), dinfo->currentRow(), dinfo->currentChannelColumn());
		doc->unlock();

		dinfo->scrollFrameBy(dinfo->editStep());

		gui::updateFrameChannel(true);
	}

	void PatternView::enterNote(int note, int octave)
	{
		if (!gui::canEdit())
			return;
		DocInfo *dinfo = gui::activeDocInfo();
		FtmDocument *doc = dinfo->doc();

		doc->lock();

		stChanNote n;
		doc->GetNoteData(dinfo->currentFrame(), dinfo->currentChannel(), dinfo->currentRow(), &n);

		if (note >= 0)
			n.Note = note;
		if (octave >= 0)
			n.Octave = octave;

		if (note == HALT || note == RELEASE)
			n.Instrument = MAX_INSTRUMENTS;
		else
			n.Instrument = dinfo->currentInstrument();

		doc->SetNoteData(dinfo->currentFrame(), dinfo->currentChannel(), dinfo->currentRow(), &n);

		doc->unlock();

		dinfo->scrollFrameBy(dinfo->editStep());
		gui::updateFrameChannel(true);
	}

	void PatternView::enterKeyAtColumn(int key)
	{
		if (!gui::canEdit())
			return;
		DocInfo *dinfo = gui::activeDocInfo();
		FtmDocument *doc = dinfo->doc();

		doc->lock();

		unsigned int playlen = doc->getFramePlayLength(dinfo->currentFrame());

		if (!doc->setColumnKey(key, dinfo->currentFrame(),
									dinfo->currentChannel(),
									dinfo->currentRow(),
									dinfo->currentChannelColumn()))
		{
			doc->unlock();
			return;
		}

		unsigned int doc_playlen = doc->getFramePlayLength(dinfo->currentFrame());

		doc->unlock();

		if (doc_playlen == playlen)
		{
			dinfo->scrollFrameBy(dinfo->editStep());
		}

		gui::updateFrameChannel(true);
	}

	void PatternView::update(bool modified)
	{
		DocInfo *dinfo = gui::activeDocInfo();
		FtmDocument *doc = dinfo->doc();

		doc->lock();

		if (modified || m_currentFrame != dinfo->currentFrame())
		{
			m_body->setModified();
		}

		unsigned int oldframe = m_currentFrame;
		unsigned int oldrow = m_currentRow;

		m_currentFrame = dinfo->currentFrame();
		m_currentRow = dinfo->currentRow();
		m_currentChannel = dinfo->currentChannel();

		verticalScrollBar()->blockSignals(true);
		verticalScrollBar()->setRange(0, doc->getFramePlayLength(m_currentFrame)-1);
		verticalScrollBar()->setValue(m_currentRow);
		verticalScrollBar()->blockSignals(false);

		doc->unlock();

		m_header->repaint();
		if (modified || (!gui::isPlaying())
				|| (!(m_currentFrame == oldframe && m_currentRow == oldrow)))
		{
			m_body->repaint();
		}
	}
}
