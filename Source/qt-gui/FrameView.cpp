#include <QPainter>
#include <QScrollBar>
#include <QMouseEvent>
#include "gui.hpp"
#include "FrameView.hpp"
#include "../linux/FtmDocument.hpp"
#include <QDebug>

namespace gui
{
	FrameView::FrameView(QWidget *parent)
		: QAbstractScrollArea(parent), font("FreeSans"), m_updating(false)
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
			dinfo->setCurrentFrame(frame);
			dinfo->setCurrentChannel(chan);
			gui::updateFrameChannel();
		}
	}

	void FrameView::paintEvent(QPaintEvent *)
	{
		const int px_unit = font.pixelSize();
		const int px_hspace = px_unit * horizontal_factor;
		const int px_vspace = px_unit * vertical_factor;
		const int y_offset = viewport()->height()/2 - px_vspace/2;

		const DocInfo *dinfo = gui::activeDocInfo();

		int currentFrame = dinfo->currentFrame();
		int currentChannel = dinfo->currentChannel();

		const int frame_y_offset = y_offset - currentFrame*px_vspace;

		QPainter p;
		p.begin(viewport());
		p.setPen(Qt::NoPen);
		p.setBrush(Qt::black);
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

		p.setPen(Qt::gray);
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
					p.setPen(QColor(0, 255, 0));
				}
				else
				{
					p.setPen(QColor(0, 178, 0));
				}

				sprintf(buf, "%02X", pattern);
				p.drawText(QRect(px_hspace*(x+1),y,px_hspace,px_vspace), buf, opt);
			}
		}

		p.setPen(Qt::gray);
		p.drawLine(px_hspace,0,px_hspace,viewport()->height());

		p.end();
	}

	void FrameView::update()
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
		viewport()->update();

		m_updating = false;
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
		if (m_updating)
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
