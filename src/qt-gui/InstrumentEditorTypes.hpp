#ifndef _INSTRUMENTEDITORTYPES_HPP_
#define _INSTRUMENTEDITORTYPES_HPP_

#include <QWidget>
#include <QTreeWidget>
#include "InstrumentEditor.hpp"

class CInstrument;
class QSpinBox;
class QLabel;
class QComboBox;
class QCheckBox;

namespace gui
{
	class InstrumentSettings
	{
	public:
		InstrumentSettings(int type, int sndchip) : m_inst_type(type), m_sndchip(sndchip){}
		virtual ~InstrumentSettings(){}

		virtual void setInstrument(FtmDocument *doc, CInstrument *inst)
		{
			m_doc = doc;
			m_inst = inst;
		}
		virtual void makeTabs(QList<InstrumentSettings_Tab> &list) = 0;
		CInstrument * instrument() const{ return m_inst; }
		FtmDocument * document() const{ return m_doc; }

		int instrumentType() const{ return m_inst_type; }
		int soundChip() const{ return m_sndchip; }
	private:
		FtmDocument *m_doc;
		CInstrument *m_inst;
		int m_inst_type;		// the allowed instrument type
		int m_sndchip;
	};

	class SequenceEditor;
	class Settings_SequencesWidget;

	class Settings_CommonSequence : public InstrumentSettings
	{
		friend class Settings_SequencesWidget;
	public:
		Settings_CommonSequence(int inst_type, int sndchip);
		void setInstrument(FtmDocument *doc, CInstrument *inst);
		void selectSequence();
	protected:
		QWidget * makeWidget();
		bool getSeqEnable(int i);
		int getSeqIndex(int i);
		void setSeqEnable(int i, bool enabled);
		void setSeqIndex(int i, int idx);
	private:
		SequenceEditor *seq_editor;
		QList<QTreeWidgetItem*> items;
		QTreeWidget *tree;
		QSpinBox *seqidx_spinbox;
	};

	class Settings_SequencesWidget : public QWidget
	{
		Q_OBJECT
	public:
		Settings_SequencesWidget(Settings_CommonSequence *w)
			: m_wgt(w){}
	private:
		Settings_CommonSequence *m_wgt;
	public slots:
		void selectSequence();
		void changeSequenceIndex(int);
		void itemChanged(QTreeWidgetItem*,int);
		void selectNextEmptySlot();
	};

	class Settings_2A03;

	class DPCMWidget : public QWidget
	{
		Q_OBJECT
	public:
		DPCMWidget(Settings_2A03 *s);
		void updateSamples(bool update_loaded=false);
	private slots:
		void load();
		void unload();
		void save();
		void import();
		void edit();
		void preview();

		void load_select(QString filename);

		void assign();
		void unassign();
		void changeOctave();
		void changePitchLoop();
		void changeSample();
		void selectAssigned();
	private:
		void setSample(int octave, int note, int idx);
		QTreeWidget * m_treewidget_assigned;
		QComboBox * m_combobox_octave;
		QComboBox * m_combobox_pitch;
		QCheckBox * m_checkbox_loop;
		QComboBox * m_combobox_sample;
		QTreeWidget * m_treewidget_loaded;
		QLabel * m_label_space;
		Settings_2A03 * m_2a03;
	};

	class Settings_2A03 : public Settings_CommonSequence
	{
	public:
		Settings_2A03();

		void setInstrument(FtmDocument *doc, CInstrument *inst);
		void makeTabs(QList<InstrumentSettings_Tab> &list);
	private:
		DPCMWidget * m_dpcm;
	};

	class Settings_VRC6 : public Settings_CommonSequence
	{
	public:
		Settings_VRC6();

		void setInstrument(FtmDocument *doc, CInstrument *inst);
		void makeTabs(QList<InstrumentSettings_Tab> &list);
	};
}

#endif
