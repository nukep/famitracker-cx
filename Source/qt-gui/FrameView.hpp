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

	public slots:
		void update();

		void scrollHorizontal(int);
		void scrollVertical(int);
	private:
		QFont font;
		bool m_updating;

		bool posToFrameChannel(QPoint p, unsigned int &frame, unsigned int &chan);
	};
}

#endif

