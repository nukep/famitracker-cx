#ifndef _PATTERNVIEW_HPP_
#define _PATTERNVIEW_HPP_

#include <QAbstractScrollArea>

namespace gui
{
	class DocInfo;

	class PatternView_Header;
	class PatternView_Body;
	class PatternView : public QAbstractScrollArea
	{
		Q_OBJECT
		friend class PatternView_Body;
	public:
		PatternView(QWidget *parent);

		void setDocInfo(DocInfo *dinfo);

		void update(bool modified=false);
		void updateStyles();

		void deleteColumn();
		void enterNote(int note, int octave);
		void insertKey();
		void backKey();
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
		DocInfo * m_dinfo;
		unsigned int m_currentFrame, m_currentRow, m_currentChannel;
	};
}

#endif

