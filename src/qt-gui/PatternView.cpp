#include <QPainter>
#include <QVBoxLayout>
#include <QScrollBar>
#include <QPainter>
#include <QWheelEvent>
#include <QBitmap>
#include <QDebug>
#include <stdio.h>
#include <string.h>
#include "PatternView.hpp"
#include "GUI.hpp"
#include "styles.hpp"
#include "famitracker-core/FtmDocument.hpp"
#include "famitracker-core/App.hpp"
#include "RowPages.hpp"
#include "pixelfonts/vincent/vincent.h"

namespace gui
{
	const int header_height = 40;

	static const float vertical_factor = 1.25f;
	static const float horizontal_factor = 2.00f;

	static bool scancodeToNote(int scancode, int octave_base, int &note, int &octave)
	{
		// TODO: mac version
		// the nice thing about scan codes is it doesn't matter what locale
		// your keyboard is.
		// TODO: other keyboard note styles

		// This maps to FastTracker 2/FL Studio style

		int ob = octave_base;

#if defined(UNIX)
		const int b1 = 10;	// top black row		(1)
		const int w1 = 24;	// top white row		(Q)
		const int b2 = 38;	// bottom black row		(A)
		const int w2 = 52;	// bottom white row		(Z)
#elif defined(WINDOWS)
		const int b1 = 2;
		const int w1 = 16;
		const int b2 = 30;
		const int w2 = 44;
#endif

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

	static QColor stylecolor(styles::Colors c, bool selected=true)
	{
		styles::color_t v = styles::color(c);
		if (!selected)
		{
			styles::color_t bg = styles::color(styles::PATTERN_BG);
			v.r = (v.r + bg.r)/2;
			v.g = (v.g + bg.g)/2;
			v.b = (v.b + bg.b)/2;
		}
		return QColor(v.r, v.g, v.b);
	}


	class PatternView_Body : public QWidget
	{
	public:

		int px_unit, px_hspace, px_vspace;
		int px_pixfont_w, px_pixfont_h;
		int colspace;
		QTextOption opt;
		QBitmap *m_pixelfont_bitmap;
		bool m_usesystemfont;

		QFont m_systemfont;
		int m_currentFrameRows;
		QPixmap *m_currentRowHighlightPixmap;
		QPixmap *m_currentRowNoFocusHighlightPixmap;
		QPixmap *m_currentRowRecordHighlightPixmap;
		QPixmap *m_primaryHighlightPixmap;
		QPixmap *m_secondaryHighlightPixmap;

		bool m_modified;

		RowPages m_rowpages;

		DocInfo * m_dinfo;

		PatternView_Body()
			: m_currentRowHighlightPixmap(NULL),
			  m_currentRowNoFocusHighlightPixmap(NULL),
			  m_currentRowRecordHighlightPixmap(NULL),
			  m_primaryHighlightPixmap(NULL),
			  m_secondaryHighlightPixmap(NULL),
			  m_modified(true),
			  m_pixelfont_bitmap(NULL)
		{
			QFont font;
			font.setPointSize(11);
			setSystemFont(font);
			opt.setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);

			redrawHighlightPixmaps();

			m_rowpages.setRequestCallback(requestCallback);
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

			if (m_pixelfont_bitmap != NULL)
				delete m_pixelfont_bitmap;
		}
		void setDocInfo(DocInfo *dinfo)
		{
			m_dinfo = dinfo;
		}
		void updateStyles()
		{
			m_rowpages.clear();
			redrawHighlightPixmaps();
			repaint();
		}
		void setPixelFont()
		{
			if (m_pixelfont_bitmap != NULL)
				delete m_pixelfont_bitmap;

			int scale = 1;

			px_pixfont_w = 8;
			px_pixfont_h = 8;

			m_pixelfont_bitmap = new QBitmap;

			QImage img(px_pixfont_w, px_pixfont_h*128, QImage::Format_ARGB32_Premultiplied);
			img.fill(Qt::white);

			const QRgb solid = qRgba(0,0,0,0);
			for (int i = 0; i < 128; i++)
			{
				int cx = 0;
				int cy = i * px_pixfont_h;
				for (int y = 0; y < px_pixfont_h; y++)
				{
					int mask = vincent_data[i][y];
					for (int x = 0; x < px_pixfont_w; x++)
					{
						bool f = (mask & (1<<(px_pixfont_w-x))) != 0;
						if (f)
						{
							img.setPixel(cx+x, cy+y, solid);
						}
					}
				}
			}

			*m_pixelfont_bitmap = QPixmap::fromImage(img);
			px_unit = px_pixfont_w;
			px_vspace = px_pixfont_h;
			colspace = px_unit/2;

			m_usesystemfont = false;
		}
		void setSystemFont(const QFont &font)
		{
			m_systemfont = font;
			QFontMetrics fm(m_systemfont);
			px_unit = fm.ascent()*0.9;
			px_hspace = px_unit * horizontal_factor;
			px_vspace = px_unit * vertical_factor;
			colspace = px_unit/2;

			m_usesystemfont = true;
		}

		static int color_lerp(int a, int b, double p)
		{
			int r = (b-a)*p + a;
			if (r > 255)
				r = 255;
			return r;
		}

		static QColor color_interpolate(QColor a, QColor b, double p)
		{
			return QColor(color_lerp(a.red(),b.red(),p),
						  color_lerp(a.green(),b.green(),p),
						  color_lerp(a.blue(),b.blue(),p),
						  color_lerp(a.alpha(),b.alpha(),p));
		}
		static QColor color_fg(int row, bool selected, int h1, int h2)
		{
			styles::Colors c;
			if (row % h2 == 0)
			{
				c = styles::PATTERN_HIGHLIGHT2_FG;
			}
			else if (row % h1 == 0)
			{
				c = styles::PATTERN_HIGHLIGHT1_FG;

			}
			else
			{
				c = styles::PATTERN_FG;
			}
			return stylecolor(c, selected);
		}

		void createHighlightPixmap(QPixmap **pix, int r, int g, int b)
		{
			if (*pix != NULL)
				delete *pix;

			*pix = new QPixmap(1, px_vspace);

			QColor bg = stylecolor(styles::PATTERN_BG);
			QColor fg = QColor(r,g,b);

			QPainter p;
			p.begin(*pix);
			{
				const double x = 1.50;
				const double y = 1.00;
				const double z = 0.65;
				QLinearGradient lg(0, 0, 0, 1);
				lg.setColorAt(0, color_interpolate(bg, fg, y));
				lg.setColorAt(1, color_interpolate(bg, fg, z));
				lg.setCoordinateMode(QGradient::ObjectBoundingMode);

				p.setPen(Qt::NoPen);
				p.setBrush(lg);
				p.drawRect(0, 0, 1, px_vspace);
				p.setPen(fg.lighter(x*100));
				p.drawPoint(0,0);
			}
			p.end();
		}

		void redrawHighlightPixmaps()
		{
			createHighlightPixmap(&m_currentRowHighlightPixmap, 0,0,160);
			createHighlightPixmap(&m_currentRowNoFocusHighlightPixmap, 64,64,50);
			createHighlightPixmap(&m_currentRowRecordHighlightPixmap, 144,46,64);

			styles::color_t h1 = styles::color(styles::PATTERN_HIGHLIGHT1_BG);
			styles::color_t h2 = styles::color(styles::PATTERN_HIGHLIGHT2_BG);
			createHighlightPixmap(&m_primaryHighlightPixmap, h2.r,h2.g,h2.b);
			createHighlightPixmap(&m_secondaryHighlightPixmap, h1.r,h1.g,h1.b);
		}

		PatternView * pvParent() const{ return m_pvParent; }

		PatternView * m_pvParent;

		// c=' ': -
		void drawChar(QPainter &p, int x, int y, char c, const QColor &col, const QColor &primary) const
		{
			if (c == ' ')
			{
				c = '-';
				p.setPen(primary);
			}
			else
			{
				p.setPen(col);
			}
			if (m_usesystemfont)
			{
				p.drawText(QRect(x, y, px_unit, px_vspace), QString(c), opt);

			}
			else
			{
				QPixmap *pixmap = m_pixelfont_bitmap;
				int cx = 0;
				int cy = int(c) * px_pixfont_h;
				p.drawPixmap(x, y, *pixmap, cx, cy, px_pixfont_w, px_pixfont_h);
			}
		}

		int columnWidth(int effColumns) const
		{
			return px_unit*6 + colspace*2 + (px_unit*3+colspace)*(effColumns+1);
		}

		// returns true if note terminates frame
		bool drawNote(QPainter &p, int x, int y, const stChanNote &n, int effColumns, const QColor &primary, bool selected, int channel) const
		{
			const QColor volcol = stylecolor(styles::PATTERN_VOL, selected);
			const QColor effcol = stylecolor(styles::PATTERN_EFFNUM, selected);
			const QColor instcol = stylecolor(styles::PATTERN_INST, selected);
			const QColor noinstcol = selected ? Qt::red : Qt::darkRed;
			bool terminate = false;

			char buf[6];

			const QColor blankcol = color_interpolate(stylecolor(styles::PATTERN_BG), primary, 0.3);

			// todo: use enumerator constant
			bool noisechannel = channel == 3;

			if (n.Note == NONE)
			{
				for (int i=0;i<3;i++)
					drawChar(p, x + px_unit*i, y, ' ', Qt::black, blankcol);
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
				if (!noisechannel)
				{
					m_dinfo->noteNotation(ni, buf);

					drawChar(p, x, y, buf[0], primary, blankcol);
					x += px_unit;

					if (buf[1] == ' ') buf[1] = '-';
					drawChar(p, x, y, buf[1], primary, blankcol);
					x += px_unit;

					sprintf(buf, "%d", n.Octave);
					drawChar(p, x, y, buf[0], primary, blankcol);
					x += px_unit + colspace;
				}
				else
				{
					ni = (ni + n.Octave*12) % 16;
					sprintf(buf, "%X-#", ni);
					for (int i = 0; i < 3; i++)
					{
						drawChar(p, x, y, buf[i], primary, blankcol);
						x += px_unit;
					}
					x += colspace;
				}
			}

			const QColor *use_instcol = &instcol;
			if (n.Instrument >= MAX_INSTRUMENTS)
			{
				sprintf(buf, "  ");
			}
			else
			{
				if (m_dinfo->doc()->GetInstrument(n.Instrument) == NULL)
				{
					// instrument does not exist
					use_instcol = &noinstcol;
				}
				sprintf(buf, "%02X", n.Instrument);
			}
			drawChar(p, x, y, buf[0], *use_instcol, blankcol);
			x += px_unit;
			drawChar(p, x, y, buf[1], *use_instcol, blankcol);
			x += px_unit + colspace;

			if (n.Vol > 0xF)
				buf[0] = ' ';
			else
				sprintf(buf, "%X", n.Vol);
			drawChar(p, x, y, buf[0], volcol, blankcol);
			x += px_unit + colspace;

			for (int i = 0; i <= effColumns; i++)
			{
				unsigned char eff = n.EffNumber[i];

				if (eff == EF_JUMP || eff == EF_SKIP || eff == EF_HALT)
					terminate = true;

				if (eff == 0)
					buf[0] = ' ';
				else
					buf[0] = EFF_CHAR[eff-1];
				drawChar(p, x, y, buf[0], effcol, blankcol);
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
				drawChar(p, x, y, buf[0], primary, blankcol);
				x += px_unit;
				drawChar(p, x, y, buf[1], primary, blankcol);
				x += px_unit + colspace;
			}

			return terminate;
		}

		// returns last row drawn ("to")
		int drawFrame(QPainter &p, unsigned int frame, int from, int to, bool selected) const
		{
			FtmDocument *d = m_dinfo->doc();
			unsigned int patternLength = d->GetPatternLength();

			unsigned int track = d->GetSelectedTrack();

			unsigned int channels = d->GetAvailableChannels();

			from = from < 0 ? 0 : from;
			to = to > (int)patternLength-1 ? (int)patternLength-1 : to;

			for (int i = from; i <= to; i++)
			{
				int y = px_vspace*i;

				const QColor rownumcol = color_fg(i, selected, d->GetHighlight(), d->GetSecondHighlight());

				char buf[6];
				p.setPen(rownumcol);
				sprintf(buf, "%02X", i);

				if (m_usesystemfont)
				{
					p.drawText(QRect(0,y,px_unit*3-colspace/2,px_vspace), buf, opt);
				}

				int x = px_unit*3;

				bool terminateFrame = false;

				for (unsigned int j = 0; j < channels; j++)
				{
					stChanNote note;

					d->GetDataAtPattern(track, d->GetPatternAtFrame(frame, j), j, i, &note);

					unsigned int effcolumns = d->GetEffColumns(j);

					terminateFrame |= drawNote(p, x, y, note, effcolumns, rownumcol, selected, j);

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
			FtmDocument *d = m_dinfo->doc();

			d->lock();

			unsigned int frame = m_dinfo->currentFrame();
			unsigned int channels = d->GetAvailableChannels();

			d->unlock();

			QPainter p;
			p.begin(this);

			unsigned int row = m_dinfo->currentRow();

			int rowWidth = 0;
			for (unsigned int i = 0; i < channels; i++)
			{
				rowWidth += columnWidth(d->GetEffColumns(i)) + colspace;
			}
			const int row_x = px_unit*3 - colspace/2;

			const int y_offset = yOffset();
			const int frame_y_offset = y_offset - row*px_vspace;

			const QColor patbg = stylecolor(styles::PATTERN_BG);
			const QColor patbg_d = patbg.darker(150);

			p.setPen(Qt::NoPen);
			p.setBrush(patbg_d);
			p.drawRect(rect());

			p.translate(0, frame_y_offset);

			if (patbg != patbg_d)
			{
				int frameheight = d->getFramePlayLength(frame) * px_vspace;
				p.setBrush(patbg);
				p.drawRect(QRect(0, 0, rowWidth+row_x, frameheight));
			}

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
			int cur_chancol = m_dinfo->currentChannelColumn();
			if (cur_chancol == 0)
				chancol_hwidth = 3;
			int chancol_x = px_unit*3 + patterncol_offset(cur_chancol);
			for (unsigned int i = 0; i < m_dinfo->currentChannel(); i++)
			{
				chancol_x += channel_width(d->GetEffColumns(i));
			}
			p.setPen(Qt::NoPen);
			p.setBrush(Qt::darkGray);
			p.drawRect(chancol_x, row*px_vspace, chancol_hwidth*px_unit, px_vspace);


			unsigned int currentframe_playlength = d->getFramePlayLength(frame);

			int from = row - y_offset / px_vspace;
			int to = row + y_offset / px_vspace;

			int highlight_to = to+1>(int)currentframe_playlength?currentframe_playlength:to+1;

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

			if (m_modified)
			{
				// pixmaps are invalidated
				m_rowpages.clear();
			}
			m_rowpages.requestRowPages(d, this, from, to, frame);
			m_rowpages.render(p, frame, from, px_vspace);

			p.resetTransform();

			p.setPen(QColor(80,80,80));
			int x = px_unit*3 - colspace/2;
			for (unsigned int i = 0; i <= channels; i++)
			{
				p.drawLine(x, 0, x, height());
				x += columnWidth(d->GetEffColumns(i)) + colspace;
			}
			p.end();

			m_modified = false;
		}

		static void requestCallback(rowpage_t *r, unsigned int rowpagesize, void *data)
		{
			const PatternView_Body *pb = (const PatternView_Body*)data;

			FtmDocument *d = pb->m_dinfo->doc();
			int width = pb->xAtChannel(d->GetAvailableChannels());
			int height = pb->px_vspace * r->row_count;
			QImage *img = new QImage(width, height, QImage::Format_ARGB32_Premultiplied);
			img->fill(Qt::NoAlpha);

			QPainter p;
			p.begin(img);

			if (pb->m_usesystemfont)
			{
				p.setFont(pb->m_systemfont);
			}

			unsigned int from = r->row_index*rowpagesize;
			int off_y = -from*pb->px_vspace;
			p.translate(0, off_y);
			pb->drawFrame(p, r->frame, from, from+r->row_count, r->selected);

			p.end();

			r->image = img;
		}

		void mouseReleaseEvent(QMouseEvent *e)
		{
			FtmDocument *d = m_dinfo->doc();

			int newchan, newcol;
			channelColAtX(e->x(), newchan, newcol);
			if (newchan >= 0)
			{
				m_dinfo->setCurrentChannel(newchan);
				m_dinfo->setCurrentChannelColumn(newcol);
			}

			if (!gui::isPlaying())
			{
				int y = (e->y() - yOffset())/px_vspace;
				if (e->y() - yOffset() < 0)
					y--;

				y += m_dinfo->currentRow();

				unsigned int frame = m_dinfo->currentFrame();

				bool hit_rowframe = true;

				if (y < 0)
				{
					// mouse is above current frame
					if (frame == 0)
					{
						hit_rowframe = false;
					}
					else
					{
						frame--;
						y += d->getFramePlayLength(frame);
						if (y < 0)
						{
							hit_rowframe = false;
						}
					}
				}
				else if (y >= (int)d->getFramePlayLength(frame))
				{
					// mouse is below current frame
					frame++;
					if (frame == m_dinfo->doc()->GetFrameCount())
					{
						hit_rowframe = false;
					}
					else
					{
						y -= d->getFramePlayLength(frame-1);
						if (y >= (int)d->getFramePlayLength(frame))
						{
							hit_rowframe = false;
						}
					}
				}

				if (hit_rowframe)
				{
					m_dinfo->setCurrentRow(y);
					m_dinfo->setCurrentFrame(frame);
				}
			}

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

			FtmDocument *d = m_dinfo->doc();

			unsigned int channels = d->GetAvailableChannels();

			for (unsigned int i = 0; i < channels; i++)
			{
				int eff = d->GetEffColumns(i);
				int w = columnWidth(eff) + colspace;
				x -= w;
				if (x < 0)
					return i;
			}

			return -1;
		}
		int xAtChannel(int channel) const
		{
			FtmDocument *d = m_dinfo->doc();

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
	};

	class PatternView_Header : public QWidget
	{
	public:
		static const int vol_x = 5;
		static const int vol_squaresize = 5;
		static const int vol_stride = vol_squaresize + 2;
		static const int vol_width = vol_stride * 16;
		static const int eff_x = vol_x + vol_width - 20;
		static const int eff_y = 6;
		static const int eff_width = 6;
		static const int eff_height = 12;
		static const int eff_stride = eff_width + 4;

		PatternView_Header(PatternView_Body *body)
			: m_body(body), channel_hovering(-1), eff_hovering(-1)
		{
			m_font.setPixelSize(12);
			setFixedHeight(header_height);

			m_gradientPixmap = new QPixmap(1, header_height);
			m_decrementEff = new QPixmap(eff_width, eff_height);
			m_incrementEff = new QPixmap(eff_width, eff_height);
			m_decrementEffHover = new QPixmap(eff_width, eff_height);
			m_incrementEffHover = new QPixmap(eff_width, eff_height);

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

			const QColor eff_unhovered(80,80,80);
			const QColor eff_hovered = Qt::white;

			m_decrementEff->fill(Qt::transparent);
			p.begin(m_decrementEff);
			drawArrow(p, true, eff_unhovered);
			p.end();

			m_decrementEffHover->fill(Qt::transparent);
			p.begin(m_decrementEffHover);
			drawArrow(p, true, eff_hovered);
			p.end();

			m_incrementEff->fill(Qt::transparent);
			p.begin(m_incrementEff);
			drawArrow(p, false, eff_unhovered);
			p.end();

			m_incrementEffHover->fill(Qt::transparent);
			p.begin(m_incrementEffHover);
			drawArrow(p, false, eff_hovered);
			p.end();

			setMouseTracking(true);
		}
		~PatternView_Header()
		{
			delete m_incrementEffHover;
			delete m_decrementEffHover;
			delete m_incrementEff;
			delete m_decrementEff;
			delete m_gradientPixmap;
		}
		void setDocInfo(DocInfo *dinfo)
		{
			m_dinfo = dinfo;
		}

		void drawArrow(QPainter &p, bool left, const QColor col)
		{
			const QColor arrow_pencolor(128,128,128);

			p.setRenderHint(QPainter::Antialiasing);
			p.setPen(arrow_pencolor);
			p.setBrush(col);
			QPolygon poly;
			int l = 0;
			int r = eff_width;
			int lx = left?r:l;
			int rx = left?l:r;
			poly << QPoint(lx, 0) << QPoint(rx, eff_height/2) << QPoint(lx, eff_height);
			p.drawPolygon(poly);
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
			int nx, ny;

			p.setPen(Qt::NoPen);


			p.setBrush(Qt::darkGray);
			nx = x+1;
			ny = y+1;

			for (int i = 0; i < 15; i++)
			{
				p.drawRect(nx, ny, vol_squaresize, vol_squaresize);

				nx += vol_stride;
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
				p.drawRect(nx, ny, vol_squaresize, vol_squaresize);

				nx += vol_stride;
			}
		}

		void paintEvent(QPaintEvent *)
		{
			FtmDocument *d = m_dinfo->doc();

			QPainter p;
			p.begin(this);

			p.drawPixmap(0,0,width(), height(), *m_gradientPixmap);

			drawHLine(p, 0,0,width());
			drawLine(p, 0,1, height()-1);

			int x = m_body->xAtChannel(0);
			drawLine(p, x, 1, height()-1);

			p.setFont(m_font);

			const core::u8 *vols = m_dinfo->volumes();

			for (unsigned int i = 0; i < d->GetAvailableChannels(); i++)
			{
				bool enabled = !gui::isMuted(i);
				int w = m_body->xAtChannel(i+1) - x;
				p.setPen(enabled ? Qt::black : Qt::red);
				QRect r = QRect(x, 0, w, height()/2).adjusted(m_font.pixelSize(), 0,0,0);
				QTextOption opt(Qt::AlignBottom | Qt::AlignLeft);
				const char *name = app::channelMap()->GetChannelName(d->getChannelsFromChip()[i]);

				p.drawText(r, name, opt);
				drawVolume(p, x + vol_x, height()/2 + 5, vols[i]);

				int effnum = d->GetEffColumns(i);

				if (effnum > 0)
				{
					bool hover = eff_hovering == 0 && channel_hovering == i;
					QPixmap *pix = hover ? m_decrementEffHover : m_decrementEff;
					p.drawPixmap(x + eff_x, eff_y, *pix);
				}
				if (effnum < MAX_EFFECT_COLUMNS-1)
				{
					bool hover = eff_hovering == 1 && channel_hovering == i;
					QPixmap *pix = hover ? m_incrementEffHover : m_incrementEff;
					p.drawPixmap(x + eff_x + eff_stride, eff_y, *pix);
				}

				x += w;
				drawLine(p, x, 1, height()-1);
			}

			p.end();
		}
		void mouseMoveEvent(QMouseEvent *e)
		{
			channel_hovering = m_body->channelAtX(e->x());
			int peh = eff_hovering;
			eff_hovering = -1;
			if (channel_hovering >= 0)
			{
				int x = e->x();
				int y = e->y();
				x = x - m_body->xAtChannel(channel_hovering) - eff_x;
				y = y - eff_y;

				if (x >= 0 && x < (eff_stride + eff_width) &&
					y >= 0 && y < eff_height)
				{
					// hovering effect arrow
					eff_hovering = x < eff_stride ? 0 : 1;
				}
			}

			if (peh != eff_hovering)
			{
				repaint();
			}
		}

		void mousePressEvent(QMouseEvent *e)
		{
			if (channel_hovering < 0)
				return;
			if (eff_hovering == -1)
			{
				gui::toggleMuted(channel_hovering);
				repaint();
				return;
			}
			FtmDocument *doc = m_dinfo->doc();

			doc->lock();
			if (eff_hovering == 0)
			{
				doc->decreaseEffColumns(channel_hovering);
			}
			else if (eff_hovering == 1)
			{
				doc->increaseEffColumns(channel_hovering);
			}
			doc->unlock();

			gui::updateFrameChannel(true);
		}
		void mouseDoubleClickEvent(QMouseEvent *e)
		{
			if (channel_hovering < 0)
				return;

			if (eff_hovering == -1)
			{
				gui::toggleSolo(channel_hovering);
				repaint();
				return;
			}

			QWidget::mouseDoubleClickEvent(e);
		}

		int channel_hovering;
		int eff_hovering;		// -1, 0, 1: none, left, right
		QPixmap *m_gradientPixmap;
		QPixmap *m_decrementEff;
		QPixmap *m_incrementEff;
		QPixmap *m_decrementEffHover;
		QPixmap *m_incrementEffHover;
		QFont m_font;
		PatternView_Body *m_body;
		DocInfo * m_dinfo;
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

	void PatternView::setDocInfo(DocInfo *dinfo)
	{
		m_dinfo = dinfo;
		m_body->setDocInfo(dinfo);
		m_header->setDocInfo(dinfo);
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
		bool editing = gui::isEditing();

		bool allownote = m_dinfo->keyRepetition() || !e->isAutoRepeat();

		if (!editing || m_dinfo->currentChannelColumn() == C_NOTE)
		{
			int scan = e->nativeScanCode();
			int octave_base = m_dinfo->currentOctave();
			int note=-1, octave=-1;
			if (scancodeToNote(scan, octave_base, note, octave))
			{
				// ignore auto repeating notes
				if (!allownote)
					return;

				bool playing = gui::isPlaying();

				if (!e->isAutoRepeat() && !playing)
					gui::auditionNote(m_dinfo->currentChannel(), octave, note);

				enterNote(note, octave);
				return;
			}
		}
		int k = e->key();
		if (k == Qt::Key_Backslash)
		{
			if (!allownote)
				return;
			// release note
			enterNote(RELEASE, 0);

			return;
		}
		if (k == Qt::Key_Apostrophe)
		{
			if (!allownote)
				return;
			// halt note
			// The original Windows FamiTracker fucks up the default, so we're using apostrophe for now
			enterNote(HALT, 0);

			return;
		}
		if (k == Qt::Key_Delete)
		{
			if (!allownote)
				return;
			deleteColumn();
			return;
		}
		if (k == Qt::Key_Insert)
		{
			// insert blank row
			if (!allownote)
				return;
			insertKey();
			return;
		}
		if (k == Qt::Key_Backspace)
		{
			// remove row
			if (!allownote)
				return;
			backKey();
			return;
		}
		if (k == Qt::Key_Tab)
		{
			m_dinfo->scrollChannelBy(1);
			gui::updateFrameChannel();
			return;
		}
		if (k == Qt::Key_Backtab)
		{
			m_dinfo->scrollChannelBy(-1);
			gui::updateFrameChannel();
			return;
		}
		if (k == Qt::Key_Left)
		{
			m_dinfo->scrollChannelColumnBy(-1);
			gui::updateFrameChannel();
			return;
		}
		if (k == Qt::Key_Right)
		{
			m_dinfo->scrollChannelColumnBy(1);
			gui::updateFrameChannel();
			return;
		}
		if (k == Qt::Key_Enter || k == Qt::Key_Return)
		{
			if (e->isAutoRepeat())
				return;

			if (e->modifiers() & Qt::AltModifier)
			{
				// play from current row
				gui::playSongAtRowConcurrent();
			}
			else if (e->modifiers() & Qt::ControlModifier)
			{
				// play row
				gui::auditionRow();
			}
			else
			{
				gui::toggleSongPlaying();
			}
			return;
		}
		if (k == Qt::Key_Space)
		{
			gui::toggleEditMode();
			return;
		}
		if (k == Qt::Key_Slash)
		{
			m_dinfo->scrollOctaveBy(-1);
			gui::updateOctave();
			return;
		}
		if (k == Qt::Key_Asterisk)
		{
			m_dinfo->scrollOctaveBy(1);
			gui::updateOctave();
			return;
		}
		if (gui::isPlaying())
			return;
		if (k == Qt::Key_Up)
		{
			m_dinfo->scrollFrameBy(-(int)m_dinfo->editStep());
			gui::updateFrameChannel();
			return;
		}
		if (k == Qt::Key_Down)
		{
			m_dinfo->scrollFrameBy(m_dinfo->editStep());
			gui::updateFrameChannel();
			return;
		}
		if (k == Qt::Key_Home)
		{
			// beginning of channel / first channel / beginning of frame
			if (m_dinfo->currentChannelColumn() > 0)
			{
				m_dinfo->setCurrentChannelColumn(0);
			}
			else if (m_dinfo->currentChannel() > 0)
			{
				m_dinfo->setCurrentChannel(0);
			}
			else
			{
				m_dinfo->setCurrentRow(0);
			}
			gui::updateFrameChannel();
			return;
		}
		if (k == Qt::Key_End)
		{
			// end of frame
			m_dinfo->setCurrentRow(m_dinfo->doc()->getFramePlayLength(m_dinfo->currentFrame()));
			gui::updateFrameChannel();
			return;
		}

		enterKeyAtColumn(k);
	}

	void PatternView::keyReleaseEvent(QKeyEvent *e)
	{
		if (e->isAutoRepeat())
			return;

		gui::auditionNoteHalt();
	}

	void PatternView::wheelEvent(QWheelEvent *e)
	{
		if (gui::isPlaying())
			return;

		bool down = e->delta() < 0;

		m_dinfo->scrollFrameBy(down ? 4 : -4);
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

		m_dinfo->setCurrentRow(verticalScrollBar()->value());
		gui::updateFrameChannel();
	}
	void PatternView::deleteColumn()
	{
		if (!gui::canEdit())
			return;
		FtmDocument *doc = m_dinfo->doc();

		doc->lock();
		doc->DeleteNote(m_dinfo->currentFrame(), m_dinfo->currentChannel(), m_dinfo->currentRow(), m_dinfo->currentChannelColumn());
		doc->unlock();

		m_dinfo->scrollFrameBy(m_dinfo->editStep());

		gui::updateFrameChannel(true);
	}

	void PatternView::enterNote(int note, int octave)
	{
		if (!gui::canEdit())
			return;
		FtmDocument *doc = m_dinfo->doc();

		doc->lock();

		stChanNote n;
		doc->GetNoteData(m_dinfo->currentFrame(), m_dinfo->currentChannel(), m_dinfo->currentRow(), &n);

		if (note >= 0)
			n.Note = note;
		if (octave >= 0)
			n.Octave = octave;

		if (note == HALT || note == RELEASE)
			n.Instrument = MAX_INSTRUMENTS;
		else
			n.Instrument = m_dinfo->currentInstrument();

		doc->SetNoteData(m_dinfo->currentFrame(), m_dinfo->currentChannel(), m_dinfo->currentRow(), &n);

		doc->unlock();

		m_dinfo->scrollFrameBy(m_dinfo->editStep());
		gui::updateFrameChannel(true);
	}

	void PatternView::insertKey()
	{
		if (!gui::canEdit())
			return;
		FtmDocument *doc = m_dinfo->doc();

		doc->lock();
		doc->InsertNote(m_dinfo->currentFrame(), m_dinfo->currentChannel(), m_dinfo->currentRow());
		doc->unlock();

		gui::updateFrameChannel(true);
	}

	void PatternView::backKey()
	{
		if (!gui::canEdit())
			return;
		FtmDocument *doc = m_dinfo->doc();

		doc->lock();
		bool removed = doc->RemoveNote(m_dinfo->currentFrame(), m_dinfo->currentChannel(), m_dinfo->currentRow());
		doc->unlock();

		if (removed)
			m_dinfo->scrollFrameBy(-1);

		gui::updateFrameChannel(true);
	}

	void PatternView::enterKeyAtColumn(int key)
	{
		if (!gui::canEdit())
			return;
		FtmDocument *doc = m_dinfo->doc();

		doc->lock();

		unsigned int playlen = doc->getFramePlayLength(m_dinfo->currentFrame());

		if (!doc->setColumnKey(key, m_dinfo->currentFrame(),
									m_dinfo->currentChannel(),
									m_dinfo->currentRow(),
									m_dinfo->currentChannelColumn()))
		{
			doc->unlock();
			return;
		}

		unsigned int doc_playlen = doc->getFramePlayLength(m_dinfo->currentFrame());

		doc->unlock();

		if (doc_playlen == playlen)
		{
			m_dinfo->scrollFrameBy(m_dinfo->editStep());
		}

		gui::updateFrameChannel(true);
	}

	void PatternView::update(bool modified)
	{
		FtmDocument *doc = m_dinfo->doc();

		doc->lock();

		if (modified || m_currentFrame != m_dinfo->currentFrame())
		{
			m_body->setModified();
		}

		unsigned int oldframe = m_currentFrame;
		unsigned int oldrow = m_currentRow;

		m_currentFrame = m_dinfo->currentFrame();
		m_currentRow = m_dinfo->currentRow();
		m_currentChannel = m_dinfo->currentChannel();

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
	void PatternView::updateStyles()
	{
		m_body->updateStyles();
	}
}
