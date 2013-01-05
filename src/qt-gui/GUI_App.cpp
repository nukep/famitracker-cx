#include <QMessageBox>
#include "GUI_App.hpp"
#include "GUI_ThreadPool.hpp"
#include "MainWindow.hpp"
#include "Settings.hpp"
#include "famitracker-core/TrackerController.hpp"
#include "famitracker-core/FtmDocument.hpp"

namespace gui
{
	App::App(QApplication *a)
		: app(a), edit_mode(false), is_playing(false), mw(NULL)
	{
		init_settings();
	}

	void App::init2(const char *sound_name)
	{
		threadPool = new ThreadPool(this);

		active_doc_index = -1;

		sink = (core::SoundSinkPlayback*)core::loadSoundSink(sound_name);
		sink->initialize(48000, 1, 150);
		sgen = new SoundGen;
		sgen->setSoundSink(sink);
		sgen->setTrackerUpdate(trackerUpdate_bootstrap, this);

		newDocument(false);

		mw = new MainWindow(this);
		mw->updateDocument();

		QVariant geom = settings()->value(SETTINGS_WINDOWGEOMETRY);
		if (geom.isValid())
		{
			mw->setGeometry(geom.toRect());
		}
		else
		{
			mw->resize(1024, 768);
		}

	}
	void App::destroy()
	{
		settings()->setValue(SETTINGS_WINDOWGEOMETRY, mw->geometry());
		destroy_settings();

		delete threadPool;
	//	delete sink;
		delete sgen;

		delete mw;
	}

	void App::spin()
	{
		mw->show();
		app->exec();
	}

	void App::trackerUpdate_bootstrap(SoundGen::rowframe_t rf, FtmDocument *doc, void *data)
	{
		App *a = (App*)data;
		a->trackerUpdate(rf, doc);
	}

	void App::trackerUpdate(const SoundGen::rowframe_t &rf, FtmDocument *doc)
	{
		// happens on non-gui thread
		DocInfo *dinfo = activeDocInfo();

		if (rf.tracker_running)
		{
			dinfo->setCurrentFrame(rf.frame);
			dinfo->setCurrentRow(rf.row);
		}
		dinfo->setVolumes(rf.volumes);

		sendUpdateEvent();

		if (rf.halt_signal)
		{
			stopSongTrackerConcurrent();
		}
	}

	void App::updateFrameChannel(bool modified)
	{
		DocInfo *dinfo = activeDocInfo();
		dinfo->setCurrentChannel(dinfo->currentChannel());
		dinfo->setCurrentChannelColumn(dinfo->currentChannelColumn());
		dinfo->setCurrentFrame(dinfo->currentFrame());
		dinfo->setCurrentRow(dinfo->currentRow());
		mw->updateFrameChannel(modified);
	}
	void App::updateOctave()
	{
		mw->updateOctave();
	}

	void App::setActiveDocument(int idx)
	{
		active_doc_index = idx;
		if (idx < 0)
			return;

		sgen->setDocument(activeDocument());
		if (mw != NULL)
		{
			mw->setDocInfo(activeDocInfo());
		}
	}

	unsigned int App::loadedDocuments()
	{
		return loaded_documents.size();
	}
	FtmDocument * App::activeDocument()
	{
		if (active_doc_index < 0)
			return NULL;

		return loaded_documents[active_doc_index].doc();
	}
	DocInfo * App::activeDocInfo()
	{
		if (active_doc_index < 0)
			return NULL;

		return &loaded_documents[active_doc_index];
	}

	void App::closeActiveDocument()
	{
		if (active_doc_index < 0)
			return;

		loaded_documents[active_doc_index].destroy();
		loaded_documents.erase(loaded_documents.begin()+active_doc_index);

		setActiveDocument(active_doc_index-1);
	}

	bool App::openDocument(core::IO *io, bool close_active)
	{
		try
		{
			FtmDocument *d = new FtmDocument;
			d->read(io);
			d->SelectTrack(0);

			if (close_active)
				closeActiveDocument();

			DocInfo a(d);

			loaded_documents.push_back(a);
			setActiveDocument(loaded_documents.size()-1);

			unmuteAll();
		}
		catch (const FtmDocumentException &e)
		{
			const QString msg = QString("Could not open file\n%1").arg(e.what());
			QMessageBox::critical(mw, "Could not open file", msg);
			return false;
		}
		return true;
	}
	void App::newDocument(bool close_active)
	{
		if (close_active)
			closeActiveDocument();

		FtmDocument *d = new FtmDocument;
		d->createEmpty();

		DocInfo a(d);

		loaded_documents.push_back(a);
		setActiveDocument(loaded_documents.size()-1);

		unmuteAll();
	}

	void App::reloadAudio()
	{
		sgen->setDocument(activeDocument());
	}

	bool App::isPlaying()
	{
		// is the tracker active? does not include auditioning.
		mtx_is_playing.lock();
		bool p = is_playing;
		mtx_is_playing.unlock();
		return p;
	}
	bool App::isEditing()
	{
		return edit_mode;
	}

	void App::setIsPlaying(bool playing)
	{
		mtx_is_playing.lock();
		is_playing = playing;
		mtx_is_playing.unlock();

		if (mw != NULL)
		{
			IsPlayingSongEvent *event = new IsPlayingSongEvent;
			event->playing = playing;
			QApplication::postEvent(mw, event);
		}
	}

	void App::sendUpdateEvent()
	{
		// called from tracker update thread

		if (mw == NULL)
			return;

		boost::unique_lock<boost::mutex> lock(m_mtx_updateEvent);

		// post an event to the main thread
		UpdateEvent *event = new UpdateEvent;
		event->mtx_updateEvent = &m_mtx_updateEvent;
		event->cond_updateEvent = &m_cond_updateEvent;
		QApplication::postEvent(mw, event);

		// wait until the gui is finished updating before resuming
		m_cond_updateEvent.wait(lock);
	}

	void App::sendCallbackEvent(mainthread_callback_t cb, void *data)
	{
		if (mw == NULL)
			return;

		CallbackEvent *event = new CallbackEvent;
		event->callback = cb;
		event->callback_data = data;
		QApplication::postEvent(mw, event);
	}

	void App::playSongConcurrent(mainthread_callback_t cb, void *data)
	{
		threadpool_playing_task t(T_PLAYSONG, cb, data);
		t.m_playing.startatrow0 = true;
		threadPool->threadpool_playing_post(t);
	}

	void App::playSongAtRowConcurrent()
	{
		threadpool_playing_task t(T_PLAYSONG);
		t.m_playing.startatrow0 = false;
		threadPool->threadpool_playing_post(t);
	}

	void App::stopSongConcurrent(mainthread_callback_t cb, void *data)
	{
		threadpool_playing_task t(T_STOPSONG, cb, data);
		threadPool->threadpool_playing_post(t);
	}

	void App::stopSongTrackerConcurrent(mainthread_callback_t cb, void *data)
	{
		threadpool_playing_task t(T_STOPSONGTRACKER, cb, data);
		threadPool->threadpool_playing_post(t);
	}

	void App::deleteSinkConcurrent(mainthread_callback_t cb, void *data)
	{
		threadpool_playing_task t(T_DELETESINK, cb, data);
		threadPool->threadpool_playing_post(t);
	}

	void App::toggleEditMode()
	{
		edit_mode = !edit_mode;
		mw->updateEditMode();
	}

	void App::auditionNote(int channel, int octave, int note)
	{
		threadpool_playing_task t(T_AUDITION);
		t.m_audition.playrow = false;
		t.m_audition.octave = octave;
		t.m_audition.note = note - C;
		t.m_audition.inst = activeDocInfo()->currentInstrument();
		t.m_audition.channel = channel;
		threadPool->threadpool_playing_post(t);
	}
	void App::auditionRow()
	{
		threadpool_playing_task t(T_AUDITION);
		t.m_audition.playrow = true;
		threadPool->threadpool_playing_post(t);
	}
	void App::auditionNoteHalt()
	{
		threadpool_playing_task t(T_HALTAUDITION);
		threadPool->threadpool_playing_post(t);
	}
	void App::auditionDPCM(const CDSample *sample)
	{

	}
	void App::setMuted(int channel, bool muted)
	{
		FtmDocument_lock_guard lock(activeDocument());

		sgen->trackerController()->setMuted(channel, muted);
	}
	bool App::isMuted(int channel)
	{
		FtmDocument_lock_guard lock(activeDocument());

		bool muted = sgen->trackerController()->muted(channel);

		return muted;
	}
	void App::unmuteAll()
	{
		FtmDocument *doc = activeDocument();
		FtmDocument_lock_guard lock(doc);

		int chans = doc->GetAvailableChannels();

		for (int i = 0; i < chans; i++)
		{
			sgen->trackerController()->setMuted(i, false);
		}
	}

	void App::toggleSolo(int channel)
	{
		// if all channels but "channel" are muted, unmute all channels
		// else, mute all channels but "channel"

		FtmDocument *doc = activeDocument();
		FtmDocument_lock_guard lock(doc);

		int chans = doc->GetAvailableChannels();

		bool hasunmuted = false;

		for (int i = 0; i < chans; i++)
		{
			if (i == channel)
				continue;

			hasunmuted |= !sgen->trackerController()->muted(i);
		}

		if (hasunmuted)
		{
			// mute all except channel
			for (int i = 0; i < chans; i++)
			{
				sgen->trackerController()->setMuted(i, i != channel);
			}
		}
		else
		{
			// unmute all
			for (int i = 0; i < chans; i++)
			{
				sgen->trackerController()->setMuted(i, false);
			}
		}
	}
}
