#include "ModuleProperties.hpp"
#include <QDebug>
#include <QMessageBox>
#include "famitracker-core/App.hpp"
#include "famitracker-core/FtmDocument.hpp"
#include "famitracker-core/FamiTrackerTypes.h"
#include "GUI_App.hpp"
#include "GUI.hpp"
#include "MainWindow.hpp"

namespace gui
{
	static inline int vibratoToIndex(int v)
	{
		switch (v)
		{
		default:
		case VIBRATO_NEW: return 0;
		case VIBRATO_OLD: return 1;
		}
	}
	static inline int indexToVibrato(int idx)
	{
		switch (idx)
		{
		default:
		case 0: return VIBRATO_NEW;
		case 1: return VIBRATO_OLD;
		}
	}

	ModulePropertiesDialog::ModulePropertiesDialog(MainWindow *parent, App *a)
		: QDialog(parent), m_app(a), m_mw(parent)
	{
		setupUi(this);

		int count = app::channelMap()->GetChipCount();

		for (int i = 0; i < count; i++)
		{
			expansionSounds->addItem(app::channelMap()->GetChipName(i));
		}

		m_doc = m_app->activeDocument();

		m_doc->lock();

		int idx = app::channelMap()->GetChipIndex(m_doc->GetExpansionChip());

		expansionSounds->setCurrentIndex(idx);

		vibrato->addItem(tr("New style (bend up & down)"));
		vibrato->addItem(tr("Old style (bend up)"));

		updateTracksList(m_doc->GetSelectedTrack());

		int vs = m_doc->GetVibratoStyle();

		vibrato->setCurrentIndex(vibratoToIndex(vs));

		m_doc->unlock();

		QObject::connect(trackTitle, SIGNAL(textChanged(QString)), this, SLOT(changeTrackTitle()));
		QObject::connect(tracksList, SIGNAL(currentRowChanged(int)), this, SLOT(selectTrack(int)));

		QObject::connect(add_button, SIGNAL(clicked()), this, SLOT(addTrack()));
		QObject::connect(remove_button, SIGNAL(clicked()), this, SLOT(removeTrack()));
		QObject::connect(moveup_button, SIGNAL(clicked()), this, SLOT(moveUpTrack()));
		QObject::connect(movedown_button, SIGNAL(clicked()), this, SLOT(moveDownTrack()));
		QObject::connect(importfile_button, SIGNAL(clicked()), this, SLOT(importTrack()));
	}
	ModulePropertiesDialog::~ModulePropertiesDialog()
	{

	}
	void ModulePropertiesDialog::accept()
	{
		m_doc->lock();

		m_doc->SelectTrack(tracksList->currentRow());

		int ident = app::channelMap()->GetChipIdent(expansionSounds->currentIndex());
		m_doc->SelectExpansionChip(ident);

		m_doc->SetVibratoStyle(indexToVibrato(vibrato->currentIndex()));

		m_doc->unlock();

		QDialog::accept();
	}

	void ModulePropertiesDialog::changeTrackTitle()
	{
		int track = tracksList->currentRow();
		m_doc->lock();
		m_doc->SetTrackTitle(track, trackTitle->text().toStdString());
		m_doc->unlock();
		tracksList->currentItem()->setText(trackTitle->text());
	}

	void ModulePropertiesDialog::selectTrack(int t)
	{
		trackTitle->blockSignals(true);
		m_doc->lock();
		trackTitle->setText(m_doc->GetTrackTitle(t));
		m_doc->unlock();
		trackTitle->blockSignals(false);
	}

	void ModulePropertiesDialog::addTrack()
	{
		m_doc->lock();

		if (m_doc->AddTrack())
		{
			updateTracksList(m_doc->GetTrackCount()-1);
		}

		m_doc->unlock();
	}
	void ModulePropertiesDialog::removeTrack()
	{
		if (tracksList->count() < 2)
			return;

		int r = QMessageBox::warning(this, "", tr("Do you want to delete this track? There is no undo for this action."), QMessageBox::Yes, QMessageBox::No);
		if (r == QMessageBox::Yes)
		{
			int track = tracksList->currentRow();

			int newTrack = track;
			if (newTrack > 0)
				newTrack--;
			else
				newTrack++;

			m_doc->lock();

			m_doc->SelectTrack(newTrack);

			m_doc->unlock();

			m_mw->updateDocument();

			// delete it
			m_doc->lock();

			m_doc->RemoveTrack(track);

			m_doc->unlock();

			newTrack = track;
			if (newTrack > 0)
				newTrack--;

			updateTracksList(newTrack);
		}
	}
	void ModulePropertiesDialog::moveUpTrack()
	{
		moveUpOrDown(true);
	}
	void ModulePropertiesDialog::moveDownTrack()
	{
		moveUpOrDown(false);
	}
	void ModulePropertiesDialog::importTrack()
	{
		gui::promptUnimplemented(this);
	}

	void ModulePropertiesDialog::moveUpOrDown(bool up)
	{
		m_doc->lock();

		int track = tracksList->currentRow();

		if (up)
			m_doc->MoveTrackUp(track);
		else
			m_doc->MoveTrackDown(track);

		track += up ? -1 : +1;

		if (track < 0)
			track = 0;
		if (track >= m_doc->GetTrackCount())
			track = m_doc->GetTrackCount()-1;

		m_doc->unlock();

		updateTracksList(track);
	}

	void ModulePropertiesDialog::updateTracksList(int current)
	{
		tracksList->blockSignals(true);
		trackTitle->blockSignals(true);

		tracksList->clear();

		unsigned int trackCount = m_doc->GetTrackCount();

		for (int i = 0; i < trackCount; i++)
		{
			tracksList->addItem(m_doc->GetTrackTitle(i));
		}

		tracksList->setCurrentRow(current);

		trackTitle->setText(m_doc->GetTrackTitle(current));

		trackTitle->blockSignals(false);
		tracksList->blockSignals(false);
	}
}
