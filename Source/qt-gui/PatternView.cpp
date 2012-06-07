#include <QPainter>
#include <QVBoxLayout>
#include <QScrollBar>
#include <QPainter>
#include "PatternView.hpp"
#include "gui.hpp"
#include "../FtmDocument.hpp"

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
		setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
		setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
	}

	void PatternView::paintEvent(QPaintEvent *)
	{
		QPainter p;
		p.begin(viewport());

		p.end();
	}

	void PatternView::update()
	{
		FtmDocument *d = gui::activeDocument();
		if (d == NULL)
			return;
	}
}
