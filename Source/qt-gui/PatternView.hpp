#ifndef _PATTERNVIEW_HPP_
#define _PATTERNVIEW_HPP_

#include <QAbstractScrollArea>

namespace gui
{
	class PatternView : public QAbstractScrollArea
	{
		Q_OBJECT
	public:
		PatternView(QWidget *parent);

		void paintEvent(QPaintEvent *);
	public slots:
		void update();
	};
}

#endif

