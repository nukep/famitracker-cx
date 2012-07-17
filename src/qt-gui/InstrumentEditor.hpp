#ifndef _INSTRUMENTEDITOR_HPP_
#define _INSTRUMENTEDITOR_HPP_

#include <QDialog>
#include "famitracker-core/Instrument.h"

class CInstrument;
class QTabWidget;

namespace gui
{
	class InstrumentSettings;
	struct InstrumentSettings_Tab
	{
		QString name;
		QWidget *widget;
	};

	class InstrumentEditor : public QDialog
	{
	public:
		InstrumentEditor(QWidget *parent);
		~InstrumentEditor();

		void setInstrument(FtmDocument *doc, CInstrument *inst);
		void removedInstrument();
	private:
		QTabWidget * m_tabwidget;
		CInstrument * m_inst;
		typedef QList<InstrumentSettings_Tab> ISTabList;
		ISTabList m_tabs;
		struct tab_range
		{
			int offset, count;
		};

		tab_range m_tabs_typeoffset[INST_COUNT];
		InstrumentSettings* m_inst_settings[INST_COUNT];
	};

}

#endif
