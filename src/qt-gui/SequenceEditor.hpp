#ifndef _SEQUENCEEDITOR_HPP_
#define _SEQUENCEEDITOR_HPP_

#include <QWidget>

class FtmDocument;
class CSequence;
class QLineEdit;
class QSpinBox;
class QLabel;
class QScrollBar;

namespace gui
{
	class SequenceEditorGraphic;

	class SequenceEditor : public QWidget
	{
		Q_OBJECT
		friend class SequenceEditorGraphic;
	public:
		SequenceEditor(int inst_type);
		~SequenceEditor();

		void setSequence(FtmDocument *doc, CSequence *seq, int type);
		void updateSequence(bool fromlineedit = false);
		void setLineEdit(QLineEdit *w);

		struct metric_t
		{
			int step_width, step_height;
			int x, y, w, h;
		};
	private:

		void parseText(const char *str);
		void sequenceToLineEdit();

		int minVal() const;
		int maxVal() const;
		int minValVisible() const;
		int maxValVisible() const;

		void posToSeqTuple(const metric_t &m, const QPoint &p, int &x, int &y) const;

		void solveMetric(metric_t &m, int items);
		void dragMouse(QPoint origin, QPoint pos, bool right);

		void drawBar(QPainter &p, int x, int y, int w, int h);
		void drawLoopReleasePoint(QPainter &p, bool loop, int point, const metric_t &o);
		void drawScale(QPainter &p, int ymin, int ymax, int items, int loop, int release, const metric_t &o);
		void drawBarGraph(QPainter &p, int ymin, int ymax);
		void drawArpGraph(QPainter &p, int ymin, int ymax);
		void drawPitchGraph(QPainter &p, int ymin, int ymax);

		SequenceEditorGraphic * m_graphic;
		QFont m_font;
		QSpinBox * m_spinbox_size;
		QLabel * m_label_duration;
		QScrollBar * m_scrollbar_arpwindow;

		FtmDocument *m_doc;
		CSequence *m_seq;
		QLineEdit *m_lineedit;
		int m_seqtype;
		int m_inst_type;
	private slots:
		void lineEditEnter();
		void changeSeqSize();
		void scrollArpWindow();
	};
}

#endif
