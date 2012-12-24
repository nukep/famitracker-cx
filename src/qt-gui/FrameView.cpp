#include <QPainter>
#include <QScrollBar>
#include <QMouseEvent>
#include "GUI.hpp"
#include "styles.hpp"
#include "FrameView.hpp"
#include "famitracker-core/FtmDocument.hpp"
#include <QDebug>

namespace gui
{
	FrameView::FrameView(QWidget *parent)
		: QAbstractScrollArea(parent), font("FreeSans"), m_updating(false),
		  m_currentFrame(0), m_currentChannel(0)
	{
		font.setPixelSize(12);
		font.setBold(true);
		horizontalScrollBar()->setPageStep(1);
		verticalScrollBar()->setPageStep(1);

		setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
		setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);

		QObject::connect(horizontalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(scrollHorizontal(int)));
		QObject::connect(verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(scrollVertical(int)));
	}

	void FrameView::resizeEvent(QResizeEvent *)
	{
	}

	static const float vertical_factor = 1.25f;
	static const float horizontal_factor = 2.00f;

	bool FrameView::posToFrameChannel(QPoint p, unsigned int &frame, unsigned int &chan)
	{
		const int px_unit = font.pixelSize();
		const int px_hspace = px_unit * horizontal_factor;
		const int px_vspace = px_unit * vertical_factor;
		const int y_offset = viewport()->height()/2 - px_vspace/2;

		const DocInfo *dinfo = gui::activeDocInfo();

		int currentFrame = dinfo->currentFrame();

		int x = p.x()/px_hspace - 1;
		int y = (p.y() - y_offset)/px_vspace + currentFrame;

		if (p.y() - y_offset < 0)
			y--;

		if (y < 0 || y >= dinfo->doc()->GetFrameCount())
			return false;

		frame = y;
		chan = x;

		return true;
	}

	void FrameView::mouseReleaseEvent(QMouseEvent *e)
	{
		DocInfo *dinfo = gui::activeDocInfo();
		if (dinfo == NULL)
			return;

		QPoint p = viewport()->mapFromParent(e->pos());

		unsigned int frame, chan;
		if (posToFrameChannel(p, frame, chan))
		{
			if (!gui::isPlaying())
			{
				dinfo->setCurrentFrame(frame);
			}
			dinfo->setCurrentChannel(chan);
			dinfo->setCurrentChannelColumn(0);
			gui::updateFrameChannel();
		}
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

	void FrameView::paintEvent(QPaintEvent *)
	{
		const int px_unit = font.pixelSize();
		const int px_hspace = px_unit * horizontal_factor;
		const int px_vspace = px_unit * vertical_factor;
		const int y_offset = viewport()->height()/2 - px_vspace/2;

		const DocInfo *dinfo = gui::activeDocInfo();

		m_currentFrame = dinfo->currentFrame();
		m_currentChannel = dinfo->currentChannel();
		int currentFrame = m_currentFrame;
		int currentChannel = m_currentChannel;

		const int frame_y_offset = y_offset - currentFrame*px_vspace;

		const QColor bg = stylecolor(styles::PATTERN_BG);

		QPainter p;
		p.begin(viewport());
		p.setPen(Qt::NoPen);
		p.setBrush(bg);
		p.drawRect(rect());

		{
			QLinearGradient lg(0, 0, 0, 1);
			lg.setColorAt(0, QColor(96, 96, 96));
			lg.setColorAt(1, QColor(64, 64, 64));
			lg.setCoordinateMode(QGradient::ObjectBoundingMode);

			p.setBrush(lg);
			p.drawRect(0, y_offset, viewport()->width(), px_vspace);
		}

		{
			p.setBrush(QColor(255,255,255,64));
			p.drawRect(px_hspace*(currentChannel+1), y_offset, px_hspace, px_vspace);
		}

		p.setFont(font);

		QTextOption opt;
		opt.setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);

		const QColor framenumcol = stylecolor(styles::PATTERN_HIGHLIGHT2_FG, true);

		p.setPen(framenumcol);
		p.setBrush(Qt::NoBrush);

		FtmDocument *d = activeDocument();

		int chans = 0;
		if (d != NULL)
		{
			 chans = d->GetAvailableChannels();
		}

		int num_frames = d->GetFrameCount();

		int viewport_height = viewport()->height();

		for (int i = 0; i < num_frames; i++)
		{
			int y = px_vspace*i+frame_y_offset;
			if (y+px_vspace < 0)
				continue;
			if (y > viewport_height)
				break;

			char buf[6];
			sprintf(buf, "%02X", i);

			p.drawText(QRect(0,y,px_hspace,px_vspace), buf, opt);
		}

		unsigned int active_patterns[MAX_CHANNELS];

		for (int i = 0; i < chans; i++)
		{
			active_patterns[i] = d->GetPatternAtFrame(currentFrame, i);
		}

		const QColor selectedpat = stylecolor(styles::PATTERN_FG, true);
		const QColor unselectedpat = stylecolor(styles::PATTERN_FG, false);


		for (int i = 0; i < num_frames; i++)
		{
			int y = px_vspace*i+frame_y_offset;
			if (y+px_vspace < 0)
				continue;
			if (y > viewport_height)
				break;
			for (int x = 0; x < chans; x++)
			{
				char buf[6];
				int pattern = d->GetPatternAtFrame(i, x);

				if (pattern == active_patterns[x])
				{
					p.setPen(selectedpat);
				}
				else
				{
					p.setPen(unselectedpat);
				}

				sprintf(buf, "%02X", pattern);
				p.drawText(QRect(px_hspace*(x+1),y,px_hspace,px_vspace), buf, opt);
			}
		}

		p.setPen(Qt::gray);
		p.drawLine(px_hspace,0,px_hspace,viewport()->height());

		p.end();
	}

	void FrameView::update(bool modified)
	{
		if (m_updating)
			return;

		DocInfo *info = activeDocInfo();
		if (info == NULL)
			return;
		FtmDocument *doc = info->doc();

		m_updating = true;

		const int pxunit = font.pixelSize();

		int chans = doc->GetAvailableChannels();

		horizontalScrollBar()->setRange(0, doc->GetAvailableChannels()-1);
		verticalScrollBar()->setRange(0, doc->GetFrameCount()-1);

		horizontalScrollBar()->setValue(info->currentChannel());
		verticalScrollBar()->setValue(info->currentFrame());

		int w_compensation = size().width() - viewport()->size().width();

		setFixedWidth(pxunit*2*(chans+1) + w_compensation);

		if (modified || (!gui::isPlaying()) || (!(m_currentFrame == info->currentFrame() && m_currentChannel == info->currentChannel())))
			viewport()->update();

		m_updating = false;
	}
	void FrameView::updateStyles()
	{
		viewport()->repaint();
	}

	void FrameView::scrollHorizontal(int i)
	{
		if (m_updating)
			return;

		DocInfo *dinfo = gui::activeDocInfo();
		if (dinfo == NULL)
			return;
		dinfo->setCurrentChannel(i);

		m_updating = true;
		gui::updateFrameChannel();
		m_updating = false;
	}

	void FrameView::scrollVertical(int i)
	{
		if (gui::isPlaying() || m_updating)
			return;

		DocInfo *dinfo = gui::activeDocInfo();
		if (dinfo == NULL)
			return;
		dinfo->setCurrentFrame(i);

		m_updating = true;
		gui::updateFrameChannel();
		m_updating = false;
	}
}
