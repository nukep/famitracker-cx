#ifndef _PATTERNVIEW_HPP_
#define _PATTERNVIEW_HPP_

#include <QAbstractScrollArea>

namespace gui
{
	class PatternView_Header;
	class PatternView_Body;
	class PatternView : public QAbstractScrollArea
	{
		Q_OBJECT
		friend class PatternView_Body;
	public:
		PatternView(QWidget *parent);

		void update(bool modified=false);

		void deleteColumn();
		void enterNote(int note, int octave);
		void enterKeyAtColumn(int key);
	protected:
		bool event(QEvent *);
		void keyPressEvent(QKeyEvent *);
		void keyReleaseEvent(QKeyEvent *);
		void wheelEvent(QWheelEvent *);
		void focusInEvent(QFocusEvent *);
		void focusOutEvent(QFocusEvent *);
		void scrollContentsBy(int dx, int dy);
	private:
		PatternView_Header * m_header;
		PatternView_Body * m_body;
		unsigned int m_currentFrame, m_currentRow, m_currentChannel;
	};
}

#endif

