#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QLineEdit>
#include <QSpinBox>
#include <QComboBox>
#include <QCheckBox>
#include <QLabel>
#include <QFileDialog>
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
		m_dpcm = new DPCMWidget(this);
	}
	void Settings_2A03::setInstrument(CInstrument *inst)
	{
		Settings_CommonSequence::setInstrument(inst);
		m_dpcm->updateSamples(true);
	}
	void Settings_2A03::makeTabs(QList<InstrumentSettings_Tab> &list)
	{
		InstrumentSettings_Tab tab;
		tab.name = QObject::tr("2A03 settings");
		tab.widget = Settings_CommonSequence::makeWidget();
		list.push_back(tab);
		tab.name = QObject::tr("DPCM samples");
		tab.widget = m_dpcm;
		list.push_back(tab);
	}

	static const char noteletters[] = "CCDDEFFGGAAB";
	static const char notesharps[] =  " # #  # # # ";
	static const int notecount = 12;

	DPCMWidget::DPCMWidget(Settings_2A03 *settings)
		: m_2a03(settings)
	{
		QHBoxLayout *hl = new QHBoxLayout;
		QGroupBox *gb_assigned = new QGroupBox(QObject::tr("Assigned samples"));
		{
			QHBoxLayout *h = new QHBoxLayout;
			{
				QVBoxLayout *v = new QVBoxLayout;
				QTreeWidget *t = new QTreeWidget;
				t->setRootIsDecorated(false);
				t->setSelectionMode(QTreeWidget::ExtendedSelection);
				{
					QStringList sl;
					sl << tr("Key") << tr("Pitch") << tr("Sample");
					t->setHeaderLabels(sl);
				}
				t->setColumnCount(3);

				for (int i = 0; i < notecount; i++)
				{
					QTreeWidgetItem *item = new QTreeWidgetItem;
					char buf[3];
					buf[0] = noteletters[i];
					buf[1] = notesharps[i];
					buf[2] = 0;
					item->setText(0, buf);
					item->setData(0, Qt::UserRole, QVariant(i));
					t->addTopLevelItem(item);
				}

				t->resizeColumnToContents(0);
				t->resizeColumnToContents(1);

				connect(t, SIGNAL(itemSelectionChanged()), this, SLOT(selectAssigned()));

				v->addWidget(t);
				m_combobox_sample = new QComboBox;
				connect(m_combobox_sample, SIGNAL(currentIndexChanged(int)), this, SLOT(changeSample()));
				v->addWidget(m_combobox_sample);
				h->addLayout(v);
				m_treewidget_assigned = t;
			}
			{
				QVBoxLayout *v = new QVBoxLayout;
				v->addWidget(new QLabel(tr("Octave")));
				m_combobox_octave = new QComboBox;
				for (int i = 0; i < 8; i++)
				{
					m_combobox_octave->addItem(QString("%1").arg(i));
				}
				m_combobox_octave->setCurrentIndex(3);
				connect(m_combobox_octave, SIGNAL(currentIndexChanged(int)), this, SLOT(changeOctave()));
				v->addWidget(m_combobox_octave);
				v->addWidget(new QLabel(tr("Pitch")));
				m_combobox_pitch = new QComboBox;
				for (int i = 0; i < 16; i++)
				{
					m_combobox_pitch->addItem(QString("%1").arg(i));
				}
				m_combobox_pitch->setCurrentIndex(15);
				connect(m_combobox_pitch, SIGNAL(currentIndexChanged(int)), this, SLOT(changePitchLoop()));
				v->addWidget(m_combobox_pitch);
				m_checkbox_loop = new QCheckBox(tr("Loop"));
				connect(m_checkbox_loop, SIGNAL(toggled(bool)), this, SLOT(changePitchLoop()));
				v->addWidget(m_checkbox_loop);
				v->addStretch();
				QPushButton *b_assign = new QPushButton("<-");
				connect(b_assign, SIGNAL(clicked()), this, SLOT(assign()));
				v->addWidget(b_assign);
				QPushButton *b_unassign = new QPushButton("->");
				connect(b_unassign, SIGNAL(clicked()), this, SLOT(unassign()));
				v->addWidget(b_unassign);
				h->addLayout(v);
			}
			gb_assigned->setLayout(h);
		}
		hl->addWidget(gb_assigned);
		QGroupBox *gb_loaded = new QGroupBox(tr("Loaded samples"));
		{
			QVBoxLayout *hv = new QVBoxLayout;
			QHBoxLayout *h = new QHBoxLayout;
			{
				QTreeWidget *t = new QTreeWidget;
				t->setRootIsDecorated(false);
				t->setSelectionMode(QTreeWidget::ExtendedSelection);
				{
					QStringList sl;
					sl << tr("#") << tr("Name") << tr("Size");
					t->setHeaderLabels(sl);
				}
				t->setColumnCount(3);

				t->resizeColumnToContents(0);
				t->resizeColumnToContents(1);

				h->addWidget(t);
				m_treewidget_loaded = t;
				connect(m_treewidget_loaded, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(preview()));
			}
			{
				QVBoxLayout *v = new QVBoxLayout;
				QPushButton *b;
				b = new QPushButton(tr("Load"));
				connect(b, SIGNAL(clicked()), this, SLOT(load()));
				v->addWidget(b);
				b = new QPushButton(tr("Unload"));
				connect(b, SIGNAL(clicked()), this, SLOT(unload()));
				v->addWidget(b);
				b = new QPushButton(tr("Save"));
				connect(b, SIGNAL(clicked()), this, SLOT(save()));
				v->addWidget(b);
				b = new QPushButton(tr("Import"));
				connect(b, SIGNAL(clicked()), this, SLOT(import()));
				v->addWidget(b);
				b = new QPushButton(tr("Edit"));
				connect(b, SIGNAL(clicked()), this, SLOT(edit()));
				v->addWidget(b);
				b = new QPushButton(tr("Preview"));
				connect(b, SIGNAL(clicked()), this, SLOT(preview()));
				v->addWidget(b);

				v->addStretch();
				h->addLayout(v);
			}
			hv->addLayout(h);
			m_label_space = new QLabel;
			hv->addWidget(m_label_space);
			gb_loaded->setLayout(hv);
		}
		hl->addWidget(gb_loaded);
		setLayout(hl);
	}

	const char *dmcfilter = "Delta modulated samples (*.dmc);;All files (*.*)";

	class CustomFileDialog : public QFileDialog
	{
	public:
		CustomFileDialog()
		{
			setFilter(tr(dmcfilter));
			setFileMode(QFileDialog::ExistingFiles);
		}
	};

	void DPCMWidget::updateSamples(bool update_loaded)
	{
		FtmDocument *doc = gui::activeDocument();

		int octave = m_combobox_octave->currentIndex();

		CInstrument2A03 *inst = (CInstrument2A03*)m_2a03->instrument();

		for (int i = 0; i < notecount; i++)
		{
			QTreeWidgetItem *item = m_treewidget_assigned->topLevelItem(i);
			int idx = inst->GetSample(octave, i) - 1;
			bool loop = inst->GetSampleLoop(octave, i);
			int pitch = inst->GetSamplePitch(octave, i) & 0x7F;

			if (idx < 0)
			{
				// no sample loaded
				item->setText(1, "-");
				item->setText(2, tr("(no sample)"));
			}
			else
			{
				QString str;
				str.sprintf("%d", pitch);
				if (loop)
					str.append(" L");
				item->setText(1, str);
				item->setText(2, doc->GetDSample(idx)->Name);
			}
			item->setData(2, Qt::UserRole, QVariant(idx));
		}

		if (update_loaded)
		{
			m_treewidget_loaded->clear();
			m_combobox_sample->blockSignals(true);
			m_combobox_sample->clear();

			m_combobox_sample->addItem(tr("(no sample)"), QVariant(-1));

			unsigned int size = 0;

			for (int i = 0; i < MAX_DSAMPLES; i++)
			{
				CDSample *s = doc->GetDSample(i);
				if (s->SampleSize == 0)
					continue;
				size += s->SampleSize;

				QTreeWidgetItem *item = new QTreeWidgetItem;
				item->setText(0, QString("%1").arg(i));
				item->setText(1, s->Name);
				item->setText(2, QString("%1").arg(s->SampleSize));
				item->setData(0, Qt::UserRole, QVariant(i));
				m_treewidget_loaded->addTopLevelItem(item);

				QString str;
				str.sprintf("%02d - %s", i, s->Name);

				m_combobox_sample->addItem(str, QVariant(i));
			}
			m_combobox_sample->blockSignals(false);

			m_treewidget_loaded->resizeColumnToContents(1);

			int total_address_space = MAX_SAMPLE_SPACE;

			int used = size / 0x400;
			int avail = total_address_space / 0x400;
			int left = avail - used;

			m_label_space->setText(QString(tr("Space used %1 kB, left %2 kB (%3 kB available)")).arg(used).arg(left).arg(avail));
		}
	}

	void DPCMWidget::load()
	{
		CustomFileDialog *f = new CustomFileDialog;
		f->exec();

		FtmDocument *doc = gui::activeDocument();

		QStringList files = f->selectedFiles();

		for (QStringList::const_iterator it = files.begin(); it != files.end(); ++it)
		{
			const QString &str = *it;
			QFileInfo fileInfo(str);

			core::FileIO file(str.toLocal8Bit(), core::IO_READ);
			int idx = doc->LoadSample(&file, fileInfo.fileName().toAscii());
			if (idx == -1)
			{
				// unable to load
				break;
			}
		}

		updateSamples(true);
	}
	void DPCMWidget::load_select(QString filename)
	{
		qDebug() << filename;

	}

	void DPCMWidget::unload()
	{
		FtmDocument *doc = gui::activeDocument();

		typedef QList<QTreeWidgetItem*> List;
		List list = m_treewidget_loaded->selectedItems();

		for (List::const_iterator it = list.begin(); it != list.end(); ++it)
		{
			QTreeWidgetItem *item = *it;
			int idx = item->data(0, Qt::UserRole).toInt();

			doc->RemoveDSample(idx);
		}
		updateSamples(true);
	}
	void DPCMWidget::save()
	{
		FtmDocument *doc = gui::activeDocument();

		typedef QList<QTreeWidgetItem*> List;
		List list = m_treewidget_loaded->selectedItems();

		for (List::const_iterator it = list.begin(); it != list.end(); ++it)
		{
			QTreeWidgetItem *item = *it;

			int idx = item->data(0, Qt::UserRole).toInt();
			CDSample *samp = doc->GetDSample(idx);

			QString path = QFileDialog::getSaveFileName(this, tr("Save As"), QString(samp->Name), tr(dmcfilter), 0, 0);
			if (path.isEmpty())
				continue;

			core::FileIO file(path.toLocal8Bit(), core::IO_WRITE);
			doc->SaveSample(&file, idx);
		}
	}
	void DPCMWidget::import()
	{

	}
	void DPCMWidget::edit()
	{

	}
	void DPCMWidget::preview()
	{
		FtmDocument *doc = gui::activeDocument();

		QTreeWidgetItem *item = m_treewidget_loaded->currentItem();
		int idx = item->data(0, Qt::UserRole).toInt();

		const CDSample *sample = doc->GetDSample(idx);

		gui::auditionDPCM(sample);
	}
	void DPCMWidget::assign()
	{
		int from_idx;
		int to_octave, to_note;
		{
			QList<QTreeWidgetItem*> fl = m_treewidget_loaded->selectedItems();
			if (fl.isEmpty())
				return;
			QTreeWidgetItem *from = fl.at(0);
			from_idx = from->data(0, Qt::UserRole).toInt();
			from->setSelected(false);
		}
		CInstrument2A03 *inst = (CInstrument2A03*)m_2a03->instrument();
		{
			QList<QTreeWidgetItem*> tl = m_treewidget_assigned->selectedItems();
			to_octave = m_combobox_octave->currentIndex();
			if (tl.isEmpty())
			{
				// the next unassigned note
				to_note = -1;
				for (int i = 0; i < 12; i++)
				{
					int s = inst->GetSample(to_octave, i) - 1;
					if (s == -1)
					{
						to_note = i;
						break;
					}
				}
				if (to_note == -1)
					return;
			}
			else
			{
				QTreeWidgetItem *to = tl.at(0);
				to_note = to->data(0, Qt::UserRole).toInt();
				to->setSelected(false);
			}
		}
		setSample(to_octave, to_note, from_idx);
		updateSamples();
	}
	void DPCMWidget::unassign()
	{
		typedef QList<QTreeWidgetItem*> List;
		List list = m_treewidget_assigned->selectedItems();

		int octave = m_combobox_octave->currentIndex();

		CInstrument2A03 *inst = (CInstrument2A03*)m_2a03->instrument();

		for (List::iterator it = list.begin(); it != list.end(); ++it)
		{
			QTreeWidgetItem *item = *it;

			int note = item->data(0, Qt::UserRole).toInt();

			inst->SetSample(octave, note, 0);

			item->setSelected(false);
		}
		updateSamples();
	}

	void DPCMWidget::changeOctave()
	{
		updateSamples();
	}
	void DPCMWidget::changePitchLoop()
	{
		int octave = m_combobox_octave->currentIndex();

		typedef QList<QTreeWidgetItem*> List;
		List list = m_treewidget_assigned->selectedItems();

		CInstrument2A03 *inst = (CInstrument2A03*)m_2a03->instrument();

		int pitch = m_combobox_pitch->currentIndex();
		bool loop = m_checkbox_loop->isChecked();

		for (List::const_iterator it = list.begin(); it != list.end(); ++it)
		{
			QTreeWidgetItem *item = *it;
			int note = item->data(0, Qt::UserRole).toInt();

			inst->SetSamplePitch(octave, note, pitch);
			inst->SetSampleLoop(octave, note, loop);
		}

		updateSamples();
	}
	void DPCMWidget::changeSample()
	{
		int octave = m_combobox_octave->currentIndex();
		int idx = m_combobox_sample->itemData(m_combobox_sample->currentIndex(), Qt::UserRole).toInt();

		typedef QList<QTreeWidgetItem*> List;
		List list = m_treewidget_assigned->selectedItems();

		for (List::const_iterator it = list.begin(); it != list.end(); ++it)
		{
			QTreeWidgetItem *item = *it;
			int note = item->data(0, Qt::UserRole).toInt();
			setSample(octave, note, idx);
		}

		updateSamples();
	}
	void DPCMWidget::selectAssigned()
	{
		QTreeWidgetItem *item = m_treewidget_assigned->currentItem();

		int octave = m_combobox_octave->currentIndex();
		int note = item->data(0, Qt::UserRole).toInt();

		CInstrument2A03 *inst = (CInstrument2A03*)m_2a03->instrument();
		int idx = inst->GetSample(octave, note) - 1;

		m_combobox_pitch->blockSignals(true);
		int pitch = (idx >= 0) ? inst->GetSamplePitch(octave, note) : 15;
		m_combobox_pitch->setCurrentIndex(pitch);
		m_combobox_pitch->blockSignals(false);

		m_checkbox_loop->blockSignals(true);
		m_checkbox_loop->setChecked((idx >= 0) ? inst->GetSampleLoop(octave, note) : false);
		m_checkbox_loop->blockSignals(false);

		m_combobox_sample->blockSignals(true);
		for (int i = 0; i < m_combobox_sample->count(); i++)
		{
			if (idx == m_combobox_sample->itemData(i, Qt::UserRole).toInt())
			{
				m_combobox_sample->setCurrentIndex(i);
				break;
			}
		}
		m_combobox_sample->blockSignals(false);
	}

	void DPCMWidget::setSample(int octave, int note, int idx)
	{
		CInstrument2A03 *inst = (CInstrument2A03*)m_2a03->instrument();
		inst->SetSample(octave, note, idx+1);
		inst->SetSamplePitch(octave, note, m_combobox_pitch->currentIndex());
		inst->SetSampleLoop(octave, note, m_checkbox_loop->isChecked());
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
