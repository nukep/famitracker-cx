#ifndef _INSTRUMENTEDITORTYPES_HPP_
#define _INSTRUMENTEDITORTYPES_HPP_

#include <QWidget>
#include <QTreeWidget>
#include "InstrumentEditor.hpp"

class CInstrument;
class QSpinBox;

namespace gui
{
	class InstrumentSettings
	{
	public:
		InstrumentSettings(int type, int sndchip) : m_inst_type(type), m_sndchip(sndchip){}

		virtual void setInstrument(CInstrument *inst)
		{
			m_inst = inst;
		}
		virtual void makeTabs(QList<InstrumentSettings_Tab> &list) = 0;
		CInstrument * instrument() const{ return m_inst; }

		int instrumentType() const{ return m_inst_type; }
		int soundChip() const{ return m_sndchip; }
	private:
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
		void setInstrument(CInstrument *inst);
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

	class Settings_2A03 : public Settings_CommonSequence
	{
	public:
		Settings_2A03();

		void setInstrument(CInstrument *inst);
		void makeTabs(QList<InstrumentSettings_Tab> &list);
	};

	class Settings_VRC6 : public Settings_CommonSequence
	{
	public:
		Settings_VRC6();

		void setInstrument(CInstrument *inst);
		void makeTabs(QList<InstrumentSettings_Tab> &list);
	};
}

#endif
