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
	public:
		PatternView(QWidget *parent);

		void update(bool modified=false);

	private:
		PatternView_Header * m_header;
		PatternView_Body * m_body;
		unsigned int m_currentFrame, m_currentRow, m_currentChannel;
	};
}

#endif

