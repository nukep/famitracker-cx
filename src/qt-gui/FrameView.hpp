#ifndef _FRAMEVIEW_HPP_
#define _FRAMEVIEW_HPP_

#include <QAbstractScrollArea>

namespace gui
{
	class FrameView : public QAbstractScrollArea
	{
		Q_OBJECT
	public:
		FrameView(QWidget *parent);

		void paintEvent(QPaintEvent *);
		void resizeEvent(QResizeEvent *);
		void mouseReleaseEvent(QMouseEvent *);

		void update(bool modified=false);
		void updateStyles();
	public slots:
		void scrollHorizontal(int);
		void scrollVertical(int);
	private:
		QFont font;
		unsigned int m_currentFrame, m_currentChannel;
		bool m_updating;

		bool posToFrameChannel(QPoint p, unsigned int &frame, unsigned int &chan);
	};
}

#endif

