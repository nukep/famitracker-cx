#ifndef _SEQUENCEEDITOR_HPP_
#define _SEQUENCEEDITOR_HPP_

#include <QWidget>

class CSequence;
class QLineEdit;

namespace gui
{
	class SequenceEditor : public QWidget
	{
		Q_OBJECT
	public:
		SequenceEditor(int inst_type);
		~SequenceEditor();
		void paintEvent(QPaintEvent *);

		void setSequence(CSequence *seq, int type);
		void setLineEdit(QLineEdit *w);
	private:
		struct metric_t
		{
			int step_width, step_height;
			int x, y, w, h;
		};

		void parseText(const char *str);
		void sequenceToLineEdit();

		int minVal();
		int maxVal();
		int minValVisible();
		int maxValVisible();

		void solveMetric(metric_t &m, int ymin, int ymax, int items, bool pitch=false);

		void drawBar(QPainter &p, int x, int y, int w, int h);
		void drawLoopReleasePoint(QPainter &p, bool loop, int point, const metric_t &o);
		void drawScale(QPainter &p, int ymin, int ymax, int items, int loop, int release, const metric_t &o);
		void drawBarGraph(QPainter &p, int ymin, int ymax);
		void drawArpGraph(QPainter &p, int ymin, int ymax);
		void drawPitchGraph(QPainter &p, int ymin, int ymax);

		CSequence *m_seq;
		QLineEdit *m_lineedit;
		int m_seqtype;
		int m_inst_type;
	private slots:
		void lineEditEnter();
	};
}

#endif
