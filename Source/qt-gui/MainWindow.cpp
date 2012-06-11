#include <QFileDialog>
#include <QDebug>
#include "MainWindow.hpp"
#include "gui.hpp"
#include "../FtmDocument.hpp"

namespace gui
{
	UpdateEvent::UpdateEvent()
		: QEvent(UPDATEEVENT)
	{

	}

	MainWindow::MainWindow()
		: m_updateCount(0)
	{
		setupUi(this);
		QObject::connect(actionNew, SIGNAL(triggered()), this, SLOT(newDoc()));
		QObject::connect(action_Open, SIGNAL(triggered()), this, SLOT(open()));
		QObject::connect(action_Save, SIGNAL(triggered()), this, SLOT(save()));
		QObject::connect(actionSave_As, SIGNAL(triggered()), this, SLOT(saveAs()));
		QObject::connect(actionE_xit, SIGNAL(triggered()), this, SLOT(quit()));

		QObject::connect(action_ViewToolbar, SIGNAL(toggled(bool)), this, SLOT(viewToolbar(bool)));
		QObject::connect(action_ViewStatusBar, SIGNAL(toggled(bool)), this, SLOT(viewStatusBar(bool)));
		QObject::connect(action_ViewControlpanel, SIGNAL(toggled(bool)), this, SLOT(viewControlpanel(bool)));
		QObject::connect(toolBar, SIGNAL(visibilityChanged(bool)), action_ViewToolbar, SLOT(setChecked(bool)));
		QObject::connect(controlPanel, SIGNAL(visibilityChanged(bool)), action_ViewControlpanel, SLOT(setChecked(bool)));

		QObject::connect(songs, SIGNAL(activated(int)), this, SLOT(setSong(int)));

		QObject::connect(incPattern, SIGNAL(clicked()), this, SLOT(incrementPattern()));
		QObject::connect(decPattern, SIGNAL(clicked()), this, SLOT(decrementPattern()));
		QObject::connect(instruments, SIGNAL(itemSelectionChanged()), this, SLOT(instrumentSelect()));
		QObject::connect(instrumentName, SIGNAL(textEdited(QString)), this, SLOT(instrumentNameChange(QString)));

		QObject::connect(speed, SIGNAL(valueChanged(int)), this, SLOT(speedTempoChange(int)));
		QObject::connect(tempo, SIGNAL(valueChanged(int)), this, SLOT(speedTempoChange(int)));

		QObject::connect(action_Play, SIGNAL(triggered()), this, SLOT(play()));
		QObject::connect(action_Stop, SIGNAL(triggered()), this, SLOT(stop()));

		updateDocument();
	}

	void MainWindow::updateFrameChannel(bool modified)
	{
		frameView->update(modified);
		patternView->update(modified);
	}

	void MainWindow::sendUpdateEvent()
	{
		// called from tracker update thread
		m_updateCountMutex.lock();
		if (m_updateCount >= 2)
		{
			// forget it
			m_updateCountMutex.unlock();
			return;
		}

		m_updateCount++;
		m_updateCountMutex.unlock();

		// post an event to the main thread
		UpdateEvent *event = new UpdateEvent;
		QApplication::postEvent(this, event);
	}

	bool MainWindow::event(QEvent *event)
	{
		if (event->type() == UPDATEEVENT)
		{
			m_updateCountMutex.lock();
			m_updateCount--;
			m_updateCountMutex.unlock();

			updateFrameChannel();
			return true;
		}
		return QMainWindow::event(event);
	}

	static void setInstrumentName(QListWidgetItem *item, int i, const char *s)
	{
		char buf[256];
		sprintf(buf, "%02X - %s", i, s);

		item->setText(buf);
	}

	void MainWindow::updateDocument()
	{
		FtmDocument *d = gui::activeDocument();

		title->setText(d->GetSongName());
		title->setCursorPosition(0);
		author->setText(d->GetSongArtist());
		author->setCursorPosition(0);
		copyright->setText(d->GetSongCopyright());
		copyright->setCursorPosition(0);

		songs->clear();
		int c = d->GetTrackCount();
		for (int i = 0; i < c; i++)
		{
			QString s(d->GetTrackTitle(i));
			songs->addItem(s);
		}

		setSong(-1);
		frameView->update();

		instruments->clear();
		int instc = d->GetInstrumentCount();
		for (int i = 0; i < 64; i++)
		{
			if (!d->IsInstrumentUsed(i))
				continue;

			CInstrument *inst = d->GetInstrument(i);

			QListWidgetItem *item = new QListWidgetItem;
			setInstrumentName(item, i, inst->GetName());

			item->setData(Qt::UserRole, qVariantFromValue((void*)inst));
			instruments->addItem(item);
		//	i++;
		}
		instrumentName->clear();
		instrumentName->setEnabled(instc > 0);
		if (instc > 0)
		{
			instruments->setCurrentRow(0);
		}
	}

	void MainWindow::newDoc()
	{
		gui::newDocument(true);

		updateDocument();
	}

	void MainWindow::open()
	{
		QString path = QFileDialog::getOpenFileName(this, tr("Open"), QString(), tr("FamiTracker files (*.ftm);;All files (*.*)"), 0, 0);
		if (path.isEmpty())
			return;

		FileIO io(path, true);

		gui::openDocument(&io, true);

		updateDocument();
	}
	void MainWindow::save()
	{
		saveAs();
	}
	void MainWindow::saveAs()
	{
		qDebug() << QFileDialog::getSaveFileName(this, tr("Save As"), QString(), tr("FamiTracker files (*.ftm);;All files (*.*)"), 0, 0);
	}

	void MainWindow::quit()
	{
		close();
	}

	void MainWindow::viewToolbar(bool v)
	{
		this->toolBar->setVisible(v);
	}
	void MainWindow::viewStatusBar(bool v)
	{
		this->statusbar->setVisible(v);
	}
	void MainWindow::viewControlpanel(bool v)
	{
		this->controlPanel->setVisible(v);
	}

	void MainWindow::setSong(int i)
	{
		DocInfo *dinfo = gui::activeDocInfo();
		FtmDocument *d = dinfo->doc();

		if (d->GetSelectedTrack() == i)
			return;

		if (i < 0)
			i = 0;

		gui::stopSong();

		d->SelectTrack(i);

		speed->blockSignals(true);
		tempo->blockSignals(true);

		speed->setValue(d->GetSongSpeed());
		tempo->setValue(d->GetSongTempo());
		rows->setValue(d->GetPatternLength());
		frames->setValue(d->GetFrameCount());
		dinfo->setCurrentChannel(0);
		dinfo->setCurrentFrame(0);
		dinfo->setCurrentRow(0);

		tempo->blockSignals(false);
		speed->blockSignals(false);

		updateFrameChannel(true);
	}

	void MainWindow::incrementPattern()
	{
		DocInfo *info = gui::activeDocInfo();
		FtmDocument *doc = info->doc();

		doc->lock();

		int current_frame = info->currentFrame();
		int current_channel = info->currentChannel();
		if (changeAll->checkState() == Qt::Checked)
		{
			for (int i = 0; i < doc->GetAvailableChannels(); i++)
			{
				doc->IncreasePattern(current_frame, i, 1);
			}
		}
		else
		{
			doc->IncreasePattern(current_frame, current_channel, 1);
		}
		frameView->update(true);

		doc->unlock();
	}
	void MainWindow::decrementPattern()
	{
		DocInfo *info = gui::activeDocInfo();
		FtmDocument *doc = info->doc();

		doc->lock();

		int current_frame = info->currentFrame();
		int current_channel = info->currentChannel();
		if (changeAll->checkState() == Qt::Checked)
		{
			for (int i = 0; i < doc->GetAvailableChannels(); i++)
			{
				doc->DecreasePattern(current_frame, i, 1);
			}
		}
		else
		{
			doc->DecreasePattern(current_frame, current_channel, 1);
		}
		frameView->update(true);

		doc->unlock();
	}
	void MainWindow::instrumentSelect()
	{
		QModelIndex idx = instruments->currentIndex();
		CInstrument *inst = (CInstrument*)idx.data(Qt::UserRole).value<void*>();
		instrumentName->setText(inst->GetName());
	//	qDebug() << instruments->currentIndex();
	}
	void MainWindow::instrumentNameChange(QString s)
	{
		QModelIndex idx = instruments->currentIndex();
		if (idx.row() < 0)
			return;

		CInstrument *inst = (CInstrument*)idx.data(Qt::UserRole).value<void*>();
		inst->SetName(s.toAscii());
		setInstrumentName(instruments->currentItem(), idx.row(), s.toAscii());
	}
	void MainWindow::speedTempoChange(int i)
	{
		FtmDocument *d = gui::activeDocument();
		d->lock();

		d->SetSongSpeed(speed->value());
		d->SetSongTempo(tempo->value());

		d->unlock();
	}
	void MainWindow::play()
	{
		gui::playSong();
	}
	void MainWindow::stop()
	{
		gui::stopSong();
	}
}
