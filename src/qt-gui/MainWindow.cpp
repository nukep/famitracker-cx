#include <QFileDialog>
#include <QDebug>
#include "MainWindow.hpp"
#include "gui.hpp"
#include "famitracker-core/FtmDocument.hpp"
#include "InstrumentEditor.hpp"

namespace gui
{
	MainWindow::MainWindow()
	{
		setupUi(this);
		instruments->setIconSize(QSize(16,16));

		speed->setRange(MIN_SPEED, MAX_SPEED);
		tempo->setRange(MIN_TEMPO, MAX_TEMPO);
		rows->setRange(1, MAX_PATTERN_LENGTH);
		frames->setRange(1, MAX_FRAMES);

		title->setMaxLength(MAX_SONGINFO_LENGTH);
		author->setMaxLength(MAX_SONGINFO_LENGTH);
		copyright->setMaxLength(MAX_SONGINFO_LENGTH);
		instrumentName->setMaxLength(MAX_INSTRUMENT_NAME_LENGTH);

		QObject::connect(actionNew, SIGNAL(triggered()), this, SLOT(newDoc()));
		actionNew->setIcon(QIcon::fromTheme("document-new"));
		QObject::connect(action_Open, SIGNAL(triggered()), this, SLOT(open()));
		action_Open->setIcon(QIcon::fromTheme("document-open"));
		QObject::connect(action_Save, SIGNAL(triggered()), this, SLOT(save()));
		action_Save->setIcon(QIcon::fromTheme("document-save"));
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
		QObject::connect(instruments, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(instrumentDoubleClicked(QModelIndex)));

		QObject::connect(speed, SIGNAL(valueChanged(int)), this, SLOT(speedTempoChange(int)));
		QObject::connect(tempo, SIGNAL(valueChanged(int)), this, SLOT(speedTempoChange(int)));
		QObject::connect(rows, SIGNAL(valueChanged(int)), this, SLOT(rowsChange(int)));
		QObject::connect(frames, SIGNAL(valueChanged(int)), this, SLOT(framesChange(int)));

		QObject::connect(title, SIGNAL(editingFinished()), this, SLOT(changeSongTitle()));
		QObject::connect(author, SIGNAL(editingFinished()), this, SLOT(changeSongAuthor()));
		QObject::connect(copyright, SIGNAL(editingFinished()), this, SLOT(changeSongCopyright()));

		QObject::connect(action_Play, SIGNAL(triggered()), this, SLOT(play()));
		action_Play->setIcon(QIcon::fromTheme("media-playback-start"));
		QObject::connect(action_Stop, SIGNAL(triggered()), this, SLOT(stop()));
		action_Stop->setIcon(QIcon::fromTheme("media-playback-stop"));
		QObject::connect(actionToggle_edit_mode, SIGNAL(triggered()), this, SLOT(toggleEditMode()));
		actionToggle_edit_mode->setIcon(QIcon::fromTheme("media-record"));

		QObject::connect(actionAdd_instrument, SIGNAL(triggered()), this, SLOT(addInstrument()));
		actionAdd_instrument->setIcon(QIcon::fromTheme("list-add"));
		QObject::connect(actionRemove_instrument, SIGNAL(triggered()), this, SLOT(removeInstrument()));
		actionRemove_instrument->setIcon(QIcon::fromTheme("list-remove"));
		QObject::connect(actionEdit_instrument, SIGNAL(triggered()), this, SLOT(editInstrument()));

		QMenu *m = new QMenu;
		m->addAction(tr("New 2A03 instrument"));

		actionAdd_instrument->setMenu(m);

		m_instrumenteditor = new InstrumentEditor(this);
	}
	MainWindow::~MainWindow()
	{
		delete m_instrumenteditor;
	}

	void MainWindow::updateFrameChannel(bool modified)
	{
		frameView->update(modified);
		patternView->update(modified);
	}

	void MainWindow::sendUpdateEvent()
	{
		// called from tracker update thread

		boost::unique_lock<boost::mutex> lock(m_mtx_updateEvent);

		// post an event to the main thread
		UpdateEvent *event = new UpdateEvent;
		QApplication::postEvent(this, event);

		// wait until the gui is finished updating before resuming
		m_cond_updateEvent.wait(lock);
	}
	void MainWindow::sendStoppedSongEvent(stopsong_callback mainthread_callback, void *data)
	{
		StoppedSongEvent *event = new StoppedSongEvent;
		event->callback = mainthread_callback;
		event->callback_data = data;
		QApplication::postEvent(this, event);
	}

	void MainWindow::updateEditMode()
	{
		bool b = gui::isEditing();
		actionToggle_edit_mode->setChecked(b);
		patternView->update();
	}
	void MainWindow::setPlaying(bool playing)
	{
		rows->setEnabled(!playing);
		frames->setEnabled(!playing);
	}

	static void doclose_cb(MainWindow *mw, void *)
	{
		mw->close();
	}

	void MainWindow::closeEvent(QCloseEvent *e)
	{
		if (gui::isPlaying())
		{
			((QEvent*)e)->ignore();
			gui::stopSongConcurrent(doclose_cb);
		}
		else
		{
			((QEvent*)e)->accept();
		}
	}

	bool MainWindow::event(QEvent *event)
	{
		if (event->type() == UPDATEEVENT)
		{
			updateFrameChannel();

			m_cond_updateEvent.notify_one();
			return true;
		}
		else if (event->type() == STOPPEDSONGEVENT)
		{
			StoppedSongEvent *e = (StoppedSongEvent*)event;

			setPlaying(false);
			if (e->callback != NULL)
			{
				(*e->callback)(this, e->callback_data);
			}
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

		title->blockSignals(true);
		author->blockSignals(true);
		copyright->blockSignals(true);

		title->setText(d->GetSongName());
		title->setCursorPosition(0);
		author->setText(d->GetSongArtist());
		author->setCursorPosition(0);
		copyright->setText(d->GetSongCopyright());
		copyright->setCursorPosition(0);

		copyright->blockSignals(false);
		author->blockSignals(false);
		title->blockSignals(false);

		songs->clear();
		int c = d->GetTrackCount();
		for (int i = 0; i < c; i++)
		{
			QString s(d->GetTrackTitle(i));
			songs->addItem(s);
		}

		songs->setCurrentIndex(0);
		setSong(-1);
		frameView->update();

		refreshInstruments();
	}

	void MainWindow::refreshInstruments()
	{
		FtmDocument *d = gui::activeDocument();

		int current = gui::activeDocInfo()->currentInstrument();

		instruments->clear();
		int instc = d->GetInstrumentCount();

		instrumentName->clear();
		instrumentName->setEnabled(instc > 0);
		for (int i = 0; i < 64; i++)
		{
			if (!d->IsInstrumentUsed(i))
				continue;

			CInstrument *inst = d->GetInstrument(i);

			QListWidgetItem *item = new QListWidgetItem;
			const char *res;
			switch (inst->GetType())
			{
			case INST_NONE:
			case INST_2A03:
				res = ":/inst/2a03";
				break;
			case INST_VRC6:
				res = ":/inst/vrc6";
				break;
			case INST_VRC7:
				res = ":/inst/vrc7";
				break;
			case INST_FDS:
				res = ":/inst/fds";
				break;
			default:
				res = NULL;
				break;
			}

			if (res != NULL)
				item->setIcon(QIcon(res));
			setInstrumentName(item, i, inst->GetName());

			item->setData(Qt::UserRole, qVariantFromValue(i));
			instruments->addItem(item);

			if (i == current)
			{
				instruments->setCurrentItem(item);
				instrumentName->setText(inst->GetName());
			}
		}
	}

	void MainWindow::newDoc_cb(MainWindow *mw, void*)
	{
		mw->m_instrumenteditor->removedInstrument();
		gui::newDocument(true);
		mw->updateDocument();
	}

	void MainWindow::open_cb(MainWindow *mw, void *data)
	{
		mw->m_instrumenteditor->removedInstrument();
		core::IO *io = (core::IO*)data;
		gui::openDocument(io, true);
		mw->updateDocument();
		delete io;
	}

	void MainWindow::newDoc()
	{
		gui::stopSongConcurrent(newDoc_cb);
	}

	void MainWindow::open()
	{
		QString path = QFileDialog::getOpenFileName(this, tr("Open"), QString(), tr("FamiTracker files (*.ftm);;All files (*.*)"), 0, 0);
		if (path.isEmpty())
			return;

		core::FileIO *io = new core::FileIO(path.toLocal8Bit(), core::IO_READ);

		gui::stopSongConcurrent(open_cb, io);
	}
	void MainWindow::save()
	{
		saveAs();
	}
	void MainWindow::saveAs()
	{
		QString path = QFileDialog::getSaveFileName(this, tr("Save As"), QString(), tr("FamiTracker files (*.ftm);;All files (*.*)"), 0, 0);
		if (path.isEmpty())
			return;

		core::FileIO *io = new core::FileIO(path.toLocal8Bit(), core::IO_WRITE);

		gui::activeDocument()->write(io);

		delete io;
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

	void MainWindow::setSong_mw_cb()
	{
		DocInfo *dinfo = gui::activeDocInfo();
		FtmDocument *d = dinfo->doc();
		int i = songs->currentIndex();

		d->SelectTrack(i);

		speed->blockSignals(true);
		tempo->blockSignals(true);
		rows->blockSignals(true);
		frames->blockSignals(true);

		speed->setValue(d->GetSongSpeed());
		tempo->setValue(d->GetSongTempo());
		rows->setValue(d->GetPatternLength());
		frames->setValue(d->GetFrameCount());
		dinfo->setCurrentChannel(0);
		dinfo->setCurrentFrame(0);
		dinfo->setCurrentRow(0);

		frames->blockSignals(false);
		rows->blockSignals(false);
		tempo->blockSignals(false);
		speed->blockSignals(false);

		updateFrameChannel(true);
	}

	void MainWindow::setSong_cb(MainWindow *mw, void*)
	{
		mw->setSong_mw_cb();
	}

	void MainWindow::setSong(int i)
	{
		DocInfo *dinfo = gui::activeDocInfo();
		FtmDocument *d = dinfo->doc();

		if (d->GetSelectedTrack() == i)
			return;

		gui::stopSongConcurrent(setSong_cb);
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

		doc->unlock();

		gui::updateFrameChannel(true);
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

		doc->unlock();

		gui::updateFrameChannel(true);
	}
	void MainWindow::instrumentSelect()
	{
		QModelIndex idx = instruments->currentIndex();
		int i = idx.data(Qt::UserRole).value<int>();
		CInstrument *inst = gui::activeDocument()->GetInstrument(i);
		if (inst == NULL)
			return;
		instrumentName->setText(inst->GetName());

		gui::activeDocInfo()->setCurrentInstrument(i);

		m_instrumenteditor->setInstrument(inst);
	}
	void MainWindow::instrumentNameChange(QString s)
	{
		QModelIndex idx = instruments->currentIndex();
		if (idx.row() < 0)
			return;

		int i = idx.data(Qt::UserRole).value<int>();
		CInstrument *inst = gui::activeDocument()->GetInstrument(i);
		inst->SetName(s.toAscii());
		setInstrumentName(instruments->currentItem(), idx.row(), s.toAscii());
	}
	void MainWindow::instrumentDoubleClicked(QModelIndex)
	{
		m_instrumenteditor->show();
	}

	void MainWindow::speedTempoChange(int i)
	{
		FtmDocument *d = gui::activeDocument();
		d->lock();

		d->SetSongSpeed(speed->value());
		d->SetSongTempo(tempo->value());

		d->unlock();
	}
	void MainWindow::rowsChange(int i)
	{
		FtmDocument *d = gui::activeDocument();
		d->lock();

		d->SetPatternLength(i);

		d->unlock();

		gui::updateFrameChannel(true);
	}
	void MainWindow::framesChange(int i)
	{
		FtmDocument *d = gui::activeDocument();
		d->lock();

		d->SetFrameCount(i);

		d->unlock();

		gui::updateFrameChannel(true);
	}

	void MainWindow::changeSongTitle()
	{
		FtmDocument *doc = gui::activeDocument();
		doc->SetSongInfo(title->text().toAscii(), doc->GetSongArtist(), doc->GetSongCopyright());
	}
	void MainWindow::changeSongAuthor()
	{
		FtmDocument *doc = gui::activeDocument();
		doc->SetSongInfo(doc->GetSongName(), author->text().toAscii(), doc->GetSongCopyright());
	}
	void MainWindow::changeSongCopyright()
	{
		FtmDocument *doc = gui::activeDocument();
		doc->SetSongInfo(doc->GetSongName(), doc->GetSongArtist(), copyright->text().toAscii());
	}

	void MainWindow::play()
	{
		gui::playSong();
	}
	void MainWindow::stop()
	{
		gui::stopSongConcurrent();
	}
	void MainWindow::toggleEditMode()
	{
		gui::toggleEditMode();
	}

	void MainWindow::addInstrument()
	{
		FtmDocument *d = gui::activeDocument();
		int inst = d->AddInstrument("New instrument", SNDCHIP_NONE);

		gui::activeDocInfo()->setCurrentInstrument(inst);

		refreshInstruments();
		patternView->update();
	}
	void MainWindow::removeInstrument()
	{
		FtmDocument *d = gui::activeDocument();

		int inst = gui::activeDocInfo()->currentInstrument();

		m_instrumenteditor->removedInstrument();

		d->RemoveInstrument(inst);

		int ni = -1;

		for (int i = inst+1; i < MAX_INSTRUMENTS; i++)
		{
			if (d->GetInstrument(i) != NULL)
			{
				ni = i;
				break;
			}
		}
		if (ni == -1)
		{
			for (int i = inst-1; i >= 0; i--)
			{
				if (d->GetInstrument(i) != NULL)
				{
					ni = i;
					break;
				}
			}
		}
		if (ni == -1)
		{
			ni = 0;
		}

		gui::activeDocInfo()->setCurrentInstrument(ni);

		refreshInstruments();
		patternView->update();
	}
	void MainWindow::editInstrument()
	{
		m_instrumenteditor->show();
	}
}
