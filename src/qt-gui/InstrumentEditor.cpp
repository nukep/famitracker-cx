#include <stdio.h>
#include <QTabWidget>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include "famitracker-core/Instrument.h"
#include "GUI.hpp"
#include "famitracker-core/FtmDocument.hpp"
#include "InstrumentEditor.hpp"
#include "SequenceEditor.hpp"
#include "InstrumentEditorTypes.hpp"

static const char *CHIP_NAMES[] = {"", "2A03", "VRC6", "VRC7", "FDS", "Namco", "Sunsoft"};

namespace gui
{
#define ADDCHIP(t) {\
	InstrumentSettings *s = new t; int idx = s->instrumentType(); \
	m_inst_settings[idx] = s; m_tabs_typeoffset[idx].offset = m_tabs.size(); \
	s->makeTabs(m_tabs); \
	m_tabs_typeoffset[idx].count = m_tabs.size() - m_tabs_typeoffset[idx].offset; \
}

	InstrumentEditor::InstrumentEditor(QWidget *parent)
		: QDialog(parent), m_inst(NULL)
	{
		setInstrument(NULL, NULL);
		m_tabwidget = new QTabWidget;

		for (int i = 0; i < INST_COUNT; i++)
		{
			m_inst_settings[i] = NULL;
		}

		ADDCHIP(Settings_2A03);
		ADDCHIP(Settings_VRC6);

		QVBoxLayout *vb = new QVBoxLayout;
		vb->setMargin(0);
		vb->addWidget(m_tabwidget);
		setLayout(vb);

		resize(640, 400);
	}
	InstrumentEditor::~InstrumentEditor()
	{
		for (int i = 0; i < INST_COUNT; i++)
		{
			if (m_inst_settings[i] != NULL)
				delete m_inst_settings[i];
		}
	}

	void InstrumentEditor::setInstrument(FtmDocument *doc, CInstrument *inst)
	{
		int oldtype = -1;
		if (m_inst != NULL)
			oldtype = m_inst->GetType();

		m_inst = inst;
		if (m_inst != NULL)
		{
			int type = inst->GetType();
			QString title = tr("Instrument Editor - %1 (%2)").arg(inst->GetName(), CHIP_NAMES[type]);
			setWindowTitle(title);

			InstrumentSettings *s = m_inst_settings[type];
			if (s != NULL)
			{
				s->setInstrument(doc, inst);
				if (oldtype != type)
				{
					const tab_range &t = m_tabs_typeoffset[type];
					m_tabwidget->clear();

					for (int i = t.offset; i < t.offset + t.count; i++)
					{
						const InstrumentSettings_Tab &tab = m_tabs[i];
						m_tabwidget->addTab(tab.widget, tab.name);
					}
				}
			}
		}
	}
	void InstrumentEditor::removedInstrument()
	{
		setInstrument(NULL, NULL);
		hide();
	}
}
