#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QLineEdit>
#include <QSpinBox>
#include <QLabel>
#include "InstrumentEditorTypes.hpp"
#include "SequenceEditor.hpp"
#include "gui.hpp"
#include "famitracker-core/Instrument.h"
#include "famitracker-core/FtmDocument.hpp"
#include <QDebug>

namespace gui
{
	void Settings_SequencesWidget::selectSequence()
	{
		m_wgt->selectSequence();
	}
	void Settings_SequencesWidget::changeSequenceIndex(int idx)
	{
		m_wgt->setSeqIndex(m_wgt->tree->currentIndex().row(), idx);

		QString num;
		num.sprintf("%d", idx);

		QTreeWidgetItem *item = m_wgt->tree->currentItem();
		item->setText(1, num);

		m_wgt->selectSequence();
	}

	void Settings_SequencesWidget::itemChanged(QTreeWidgetItem *item, int i)
	{
		if (i == 0)
		{
			// check box
			bool enabled = item->checkState(i) == Qt::Checked;

			int row = item->data(0, Qt::UserRole).toInt();

			m_wgt->setSeqEnable(row, enabled);
		}
	}
	void Settings_SequencesWidget::selectNextEmptySlot()
	{
		FtmDocument *doc = gui::activeDocument();
		int idx = doc->GetFreeSequence(m_wgt->soundChip(), m_wgt->tree->currentIndex().row());
		m_wgt->seqidx_spinbox->setValue(idx);
	}

	Settings_CommonSequence::Settings_CommonSequence(int inst_type, int sndchip)
		: InstrumentSettings(inst_type, sndchip)
	{
		seq_editor = new SequenceEditor(inst_type);
	}
	void Settings_CommonSequence::setInstrument(CInstrument *inst)
	{
		InstrumentSettings::setInstrument(inst);

		tree->blockSignals(true);

		for (int i = 0; i < items.size(); i++)
		{
			items[i]->setCheckState(0, getSeqEnable(i)?Qt::Checked:Qt::Unchecked );
			QString str;
			str.sprintf("%d", getSeqIndex(i));
			items[i]->setText(1, str);
		}

		tree->blockSignals(false);

		selectSequence();
	}
	void Settings_CommonSequence::selectSequence()
	{
		FtmDocument *doc = gui::activeDocument();

		int type = tree->currentIndex().row();
		if (type < 0) type = 0;
		int idx = getSeqIndex(type);

		CSequence *s = doc->GetSequence(soundChip(), idx, type);

		seq_editor->setSequence(s, type);

		seqidx_spinbox->blockSignals(true);
		seqidx_spinbox->setValue(idx);
		seqidx_spinbox->blockSignals(false);
	}

#define SEQFUNC(var, type, func) ((type*)var)->func

	bool Settings_CommonSequence::getSeqEnable(int i)
	{
		switch (instrumentType())
		{
		case INST_2A03:
			return SEQFUNC(instrument(), CInstrument2A03, GetSeqEnable)(i) != 0;
		case INST_VRC6:
			return SEQFUNC(instrument(), CInstrumentVRC6, GetSeqEnable)(i) != 0;
		default:
			return false;
		}
	}
	int Settings_CommonSequence::getSeqIndex(int i)
	{
		switch (instrumentType())
		{
		case INST_2A03:
			return SEQFUNC(instrument(), CInstrument2A03, GetSeqIndex)(i);
		case INST_VRC6:
			return SEQFUNC(instrument(), CInstrumentVRC6, GetSeqIndex)(i);
		default:
			return 0;
		}
	}
	void Settings_CommonSequence::setSeqEnable(int i, bool enabled)
	{
		switch (instrumentType())
		{
		case INST_2A03:
			SEQFUNC(instrument(), CInstrument2A03, SetSeqEnable)(i, enabled?1:0);
			break;
		case INST_VRC6:
			SEQFUNC(instrument(), CInstrumentVRC6, SetSeqEnable)(i, enabled?1:0);
			break;
		default:
			break;
		}
	}
	void Settings_CommonSequence::setSeqIndex(int i, int idx)
	{
		switch (instrumentType())
		{
		case INST_2A03:
			SEQFUNC(instrument(), CInstrument2A03, SetSeqIndex)(i, idx);
			break;
		case INST_VRC6:
			SEQFUNC(instrument(), CInstrumentVRC6, SetSeqIndex)(i, idx);
			break;
		default:
			break;
		}
	}

	QWidget * Settings_CommonSequence::makeWidget()
	{
		Settings_SequencesWidget *w = new Settings_SequencesWidget(this);
		QHBoxLayout *hb = new QHBoxLayout;
		w->setLayout(hb);

		QGroupBox *inst_s = new QGroupBox(QWidget::tr("Instrument settings"));
		QGroupBox *seq_e = new QGroupBox(QWidget::tr("Sequence editor"));

		{
			int row = 0;
			const int colspan = 2;
			QGridLayout *inst_s_l = new QGridLayout;
			QTreeWidget *l = new QTreeWidget;
			tree = l;
			l->setColumnCount(3);
			QStringList sl;
			sl << "" << "#" << "Effect name";
			l->setHeaderLabels(sl);
			l->setRootIsDecorated(false);

			int itemrow = 0;

			QTreeWidgetItem *item = new QTreeWidgetItem;
			item->setCheckState(0, Qt::Unchecked);
			item->setText(1, "0");
			item->setText(2, QObject::tr("Volume"));
			item->setData(0, Qt::UserRole, itemrow++);
			l->addTopLevelItem(item);
			items.push_back(item);

			item = new QTreeWidgetItem(*item);
			item->setText(2, QObject::tr("Arpeggio"));
			item->setData(0, Qt::UserRole, itemrow++);
			l->addTopLevelItem(item);
			items.push_back(item);

			item = new QTreeWidgetItem(*item);
			item->setText(2, QObject::tr("Pitch"));
			item->setData(0, Qt::UserRole, itemrow++);
			l->addTopLevelItem(item);
			items.push_back(item);

			item = new QTreeWidgetItem(*item);
			item->setText(2, QObject::tr("Hi-pitch"));
			item->setData(0, Qt::UserRole, itemrow++);
			l->addTopLevelItem(item);
			items.push_back(item);

			item = new QTreeWidgetItem(*item);
			item->setText(2, QObject::tr(instrumentType()==INST_2A03?"Duty / Noise":"Pulse Width"));
			item->setData(0, Qt::UserRole, itemrow++);
			l->addTopLevelItem(item);
			items.push_back(item);

			l->resizeColumnToContents(0);
			l->resizeColumnToContents(1);
			l->setCurrentItem(items[0]);
			inst_s_l->addWidget(l, row++, 0, 1, colspan);

			QPushButton *nextslot = new QPushButton;
			// prevents activating the button when pressing enter anywhere in the dialog
			nextslot->setAutoDefault(false);
			QObject::connect(nextslot, SIGNAL(clicked()), w, SLOT(selectNextEmptySlot()));
			nextslot->setText(QWidget::tr("Select next empty slot"));
			inst_s_l->addWidget(nextslot, row++, 0, 1, colspan);

			inst_s_l->addWidget(new QLabel(QWidget::tr("Sequence #")), row, 0);

			seqidx_spinbox = new QSpinBox;
			seqidx_spinbox->setMinimum(0);
			seqidx_spinbox->setMaximum(MAX_SEQUENCES);
			inst_s_l->addWidget(seqidx_spinbox, row++, 1);

			inst_s_l->setColumnStretch(0, 2);
			inst_s_l->setColumnStretch(1, 1);

			inst_s->setLayout(inst_s_l);
		}
		{
			QVBoxLayout *seq_e_vb = new QVBoxLayout;
			QLineEdit *seq_e_le = new QLineEdit;
			seq_editor->setLineEdit(seq_e_le);
			seq_e_vb->addWidget(seq_editor);
			seq_e_vb->addWidget(seq_e_le);

			seq_e->setLayout(seq_e_vb);
		}

		QObject::connect(tree, SIGNAL(itemSelectionChanged()), w, SLOT(selectSequence()));
		QObject::connect(tree, SIGNAL(itemChanged(QTreeWidgetItem*,int)), w, SLOT(itemChanged(QTreeWidgetItem*,int)));
		QObject::connect(seqidx_spinbox, SIGNAL(valueChanged(int)), w, SLOT(changeSequenceIndex(int)));

		hb->addWidget(inst_s, 0);
		hb->addWidget(seq_e, 1);

		return w;
	}

	Settings_2A03::Settings_2A03()
		: Settings_CommonSequence(INST_2A03, SNDCHIP_NONE)
	{
	}
	void Settings_2A03::setInstrument(CInstrument *inst)
	{
		Settings_CommonSequence::setInstrument(inst);
	}
	void Settings_2A03::makeTabs(QList<InstrumentSettings_Tab> &list)
	{
		InstrumentSettings_Tab tab;
		tab.name = "2A03 settings";
		tab.widget = Settings_CommonSequence::makeWidget();
		list.push_back(tab);
	}
	Settings_VRC6::Settings_VRC6()
		: Settings_CommonSequence(INST_VRC6, SNDCHIP_VRC6)
	{
	}
	void Settings_VRC6::setInstrument(CInstrument *inst)
	{
		Settings_CommonSequence::setInstrument(inst);
	}
	void Settings_VRC6::makeTabs(QList<InstrumentSettings_Tab> &list)
	{
		InstrumentSettings_Tab tab;
		tab.name = "VRC6 settings";
		tab.widget = Settings_CommonSequence::makeWidget();
		list.push_back(tab);
	}
}
