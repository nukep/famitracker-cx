#include "PatternView.hpp"
#include <QPainter>
#include <QVBoxLayout>
#include <QScrollBar>

namespace gui
{
	class PatternView_Header : public QWidget
	{

	};

	class PatternView_Body : public QWidget
	{

	};

	PatternView::PatternView(QWidget *parent)
		: QAbstractScrollArea(parent)
	{
		verticalScrollBar()->setRange(0, 31);
		horizontalScrollBar()->setRange(0,34);
	}
}
