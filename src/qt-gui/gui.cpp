#include <QApplication>
#include <QFile>
#include <vector>
#include <QDebug>
#include <QThread>
#include <boost/thread.hpp>
#include "gui.hpp"
#include "MainWindow.hpp"
#include "famitracker-core/FtmDocument.hpp"
#include "famitracker-core/sound.hpp"
#include "famitracker-core/TrackerController.hpp"
#include "famitracker-core/App.hpp"

namespace gui
{
	DocInfo::DocInfo(FtmDocument *d)
		: m_doc(d), m_currentChannel(0), m_currentFrame(0), m_currentRow(0),
		  m_currentChannelColumn(0), m_currentInstrument(0), m_currentOctave(3),
		  m_step(1), m_keyrepetition(false),
		  m_notesharps(true)
	{
		memset(m_vols, 0, sizeof(m_vols));
	}
	void DocInfo::destroy()
	{
		delete m_doc;
	}
	void DocInfo::setCurrentFrame(unsigned int frame)
	{
		doc()->lock();
		if (frame >= doc()->GetFrameCount())
			frame = doc()->GetFrameCount()-1;
		doc()->unlock();

		m_currentFrame = frame;
		setCurrentRow(m_currentRow);
	}
	void DocInfo::setCurrentChannel(unsigned int chan)
	{
		doc()->lock();
		unsigned int availchan = doc()->GetAvailableChannels();
		doc()->unlock();

		if (chan >= availchan)
			return;
		m_currentChannel = chan;

		int patcols = patternColumns(chan);
		if (m_currentChannelColumn >= patcols)
		{
			m_currentChannelColumn = patcols-1;
		}
	}
	void DocInfo::setCurrentChannelColumn(unsigned int col)
	{
		m_currentChannelColumn = col;
	}
	void DocInfo::setCurrentChannelColumn_pos(unsigned int col)
	{
		doc()->lock();
		unsigned int availchan = doc()->GetAvailableChannels();
		doc()->unlock();

		int c = col;
		for (unsigned int i = 0; i < availchan; i++)
		{
			int patcols = patternColumns(i);
			if (c - patcols < 0)
			{
				// we're on the right channel
				setCurrentChannelColumn(c);
				setCurrentChannel(i);
				return;
			}
			c -= patcols;
		}
		// at this point, nothing is set
	}

	void DocInfo::setCurrentRow(unsigned int row)
	{
		FtmDocument_lock_guard lock(doc());

		if (row >= doc()->getFramePlayLength(m_currentFrame))
			row = doc()->getFramePlayLength(m_currentFrame)-1;
		m_currentRow = row;
	}
	void DocInfo::setVolumes(const core::u8 *vols)
	{
		FtmDocument_lock_guard lock(doc());

		memcpy(m_vols, vols, sizeof(core::u8)*doc()->GetAvailableChannels());
	}

	void DocInfo::scrollFrameBy(int delta)
	{
		int f = currentFrame();
		int r = currentRow();
		r += delta;

		doc()->lock();

		if (r < 0)
		{
			if (f == 0)
			{
				// wrap to the end
				f = doc()->GetFrameCount()-1;
			}
			else
			{
				f--;
			}
			r = doc()->getFramePlayLength(f) + r;
		}
		else if (r >= doc()->getFramePlayLength(f))
		{
			r = r - doc()->getFramePlayLength(f);
			f++;
			if (f >= doc()->GetFrameCount())
			{
				// wrap to the beginning
				f = 0;
			}
		}

		doc()->unlock();

		setCurrentFrame(f);
		setCurrentRow(r);
	}
	void DocInfo::scrollChannelBy(int delta)
	{
		int ch = m_currentChannel + delta;

		FtmDocument_lock_guard lock(doc());

		while (ch < 0)
		{
			ch += doc()->GetAvailableChannels();
		}
		while (ch >= doc()->GetAvailableChannels())
		{
			ch -= doc()->GetAvailableChannels();
		}
		m_currentChannel = ch;
		m_currentChannelColumn = 0;
	}

	void DocInfo::scrollChannelColumnBy(int delta)
	{
		if (delta == 0)
			return;
		int col = currentChannelColumn_pos() + delta;
		int total_patcol = 0;

		doc()->lock();
		unsigned int availchan = doc()->GetAvailableChannels();
		doc()->unlock();

		for (unsigned int i = 0; i < availchan; i++)
		{
			total_patcol += patternColumns(i);
		}
		while (col < 0)
		{
			col += total_patcol;
		}
		while (col >= total_patcol)
		{
			col -= total_patcol;
		}

		setCurrentChannelColumn_pos(col);
	}
	void DocInfo::scrollOctaveBy(int delta)
	{
		if (delta < 0 && (-delta > m_currentOctave))
		{
			// will go below 0
			m_currentOctave = 0;
		}
		else
		{
			m_currentOctave += delta;
			if (m_currentOctave > 7)
				m_currentOctave = 7;
		}
	}

	unsigned int DocInfo::currentChannelColumn_pos() const
	{
		unsigned int c = currentChannelColumn();
		for (unsigned int i = 0; i < currentChannel(); i++)
		{
			c += patternColumns(i);
		}
		return c;
	}

	unsigned int DocInfo::patternColumns(unsigned int chan) const
	{
		FtmDocument_lock_guard lock(doc());
		int effcol = doc()->GetEffColumns(chan);
		return C_EFF_NUM + C_EFF_COL_COUNT*(effcol+1);
	}

	void DocInfo::noteNotation(unsigned int note, char *out)
	{
		static const char notesharp_letters[] = "CCDDEFFGGAAB";
		static const char notesharp_sharps[] =  " # #  # # # ";
		static const char noteflat_letters[] =  "CDDEEFGGAABB";
		static const char noteflat_flats[] =    " b b  b b b ";

		if (note >= 12)
		{
			// noticeably errors
			out[0] = '!';
			out[1] = '!';
			return;
		}

		if (m_notesharps)
		{
			out[0] = notesharp_letters[note];
			out[1] = notesharp_sharps[note];
		}
		else
		{
			out[0] = noteflat_letters[note];
			out[1] = noteflat_flats[note];
		}
	}

	typedef std::vector<DocInfo> DocsList;
	DocsList loaded_documents;
	int active_doc_index;
	SoundGen *sgen;
	core::SoundSinkPlayback *sink;

	QApplication *app;
	MainWindow *mw;

	bool edit_mode = false;
	bool is_playing = false;
	boost::mutex mtx_is_playing;

	static void trackerUpdate(SoundGen::rowframe_t rf, FtmDocument *doc)
	{
		// happens on non-gui thread
		DocInfo *dinfo = activeDocInfo();

		if (rf.tracker_running)
		{
			dinfo->setCurrentFrame(rf.frame);
			dinfo->setCurrentRow(rf.row);
		}
		dinfo->setVolumes(rf.volumes);

		mw->sendUpdateEvent();
	}

	void init(int &argc, char **argv)
	{
		app = new QApplication(argc, argv);
		app->setApplicationName(QApplication::tr("FamiTracker"));
		app->setWindowIcon(QIcon(":/appicon"));
	}

	void init_2(const char *sound_name)
	{
		active_doc_index = -1;

		sink = (core::SoundSinkPlayback*)core::loadSoundSink(sound_name);
		sink->initialize(48000, 1, 150);
		sgen = new SoundGen;
		sgen->setSoundSink(sink);
		sgen->setTrackerUpdate(trackerUpdate);

		newDocument(false);

		mw = new MainWindow;
		mw->updateDocument();
		mw->resize(1024, 768);
	}
	void destroy()
	{
		delete sink;
		delete sgen;

		delete mw;
		delete app;
	}
	void spin()
	{
		mw->show();
		app->exec();
	}

	void updateFrameChannel(bool modified)
	{
		DocInfo *dinfo = activeDocInfo();
		dinfo->setCurrentChannel(dinfo->currentChannel());
		dinfo->setCurrentChannelColumn(dinfo->currentChannelColumn());
		dinfo->setCurrentFrame(dinfo->currentFrame());
		dinfo->setCurrentRow(dinfo->currentRow());
		mw->updateFrameChannel(modified);
	}
	void updateOctave()
	{
		mw->updateOctave();
	}

	static void setActiveDocument(int idx)
	{
		active_doc_index = idx;
		if (idx < 0)
			return;

		sgen->setDocument(activeDocument());
	}

	unsigned int loadedDocuments()
	{
		return loaded_documents.size();
	}
	FtmDocument * activeDocument()
	{
		if (active_doc_index < 0)
			return NULL;

		return loaded_documents[active_doc_index].doc();
	}
	DocInfo * activeDocInfo()
	{
		if (active_doc_index < 0)
			return NULL;

		return &loaded_documents[active_doc_index];
	}

	void closeActiveDocument()
	{
		if (active_doc_index < 0)
			return;

		loaded_documents[active_doc_index].destroy();
		loaded_documents.erase(loaded_documents.begin()+active_doc_index);

		setActiveDocument(active_doc_index-1);
	}

	void openDocument(core::IO *io, bool close_active)
	{
		if (close_active)
			closeActiveDocument();

		FtmDocument *d = new FtmDocument;
		d->read(io);
		d->SelectTrack(0);

		DocInfo a(d);

		loaded_documents.push_back(a);
		setActiveDocument(loaded_documents.size()-1);

		unmuteAll();
	}
	void newDocument(bool close_active)
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

	bool isPlaying()
	{
		// is the tracker active? does not include auditioning.
		mtx_is_playing.lock();
		bool p = is_playing;
		mtx_is_playing.unlock();
		return p;
	}
	bool isEditing()
	{
		return edit_mode;
	}

	boost::mutex mtx_stopping_song;
	boost::thread *stopping_song_thread = NULL;

	void playSong()
	{
		if (!mtx_stopping_song.try_lock())
		{
			// don't immediately play the song after the song stops
			return;
		}
		boost::lock_guard<boost::mutex> lock(mtx_is_playing);

		if (mw != NULL)
			mw->setPlaying(true);

		const DocInfo *dinfo = activeDocInfo();

		sgen->trackerController()->startAt(dinfo->currentFrame(), 0);

		sgen->startTracker();
		is_playing = true;

		mtx_stopping_song.unlock();
	}

	void sink_block()
	{
		sink->blockUntilStopped();
	}

	void stopSong()
	{
		boost::lock_guard<boost::mutex> lock(mtx_is_playing);
		sgen->stopTracker();

		if (mw != NULL)
			mw->setPlaying(false);

		is_playing = false;
	}

	static void stopsong_thread(void (*mainthread_callback)(MainWindow *, void*), void *data)
	{
		mtx_stopping_song.lock();

		sgen->stopTracker();
		sink->blockUntilStopped();
		sink->blockUntilTimerEmpty();

		mtx_is_playing.lock();
		is_playing = false;
		mtx_is_playing.unlock();

		if (mw != NULL)
		{
			mw->sendStoppedSongEvent(mainthread_callback, data);
		}

		mtx_stopping_song.unlock();
	}

	static void stopsongtracker_thread(void (*mainthread_callback)(MainWindow *, void*), void *data)
	{
		mtx_stopping_song.lock();

		sgen->stopTracker();
		sgen->blockUntilTrackerStopped();

		mtx_is_playing.lock();
		is_playing = false;
		mtx_is_playing.unlock();

		if (mw != NULL)
		{
			mw->sendStoppedSongEvent(mainthread_callback, data);
		}

		mtx_stopping_song.unlock();
	}
	static void stopsong_qevent_thread(QEvent *e)
	{
		mtx_stopping_song.lock();

		sgen->stopTracker();
		sink->blockUntilStopped();
		sink->blockUntilTimerEmpty();

		mtx_is_playing.lock();
		is_playing = false;
		mtx_is_playing.unlock();

		QApplication::postEvent(mw, e);

		mtx_stopping_song.unlock();
	}

	void stopSongConcurrent(QEvent *event)
	{
		if (stopping_song_thread != NULL)
			delete stopping_song_thread;

		stopping_song_thread = new boost::thread(stopsong_qevent_thread, event);
	}
	void stopSongConcurrent(void (*mainthread_callback)(MainWindow *, void*), void *data)
	{
		// stop the song and the sound sink without deadlocking the main thread
		// should only be called from the main thread

		if (stopping_song_thread != NULL)
			delete stopping_song_thread;

		stopping_song_thread = new boost::thread(stopsong_thread, mainthread_callback, data);
	}
	void stopSongConcurrent()
	{
		stopSongConcurrent(NULL, NULL);
	}

	void stopSongTrackerConcurrent(void (*mainthread_callback)(MainWindow *, void*), void *data)
	{
		// stop the song tracker without deadlocking the main thread
		// should only be called from the main thread

		if (stopping_song_thread != NULL)
			delete stopping_song_thread;

		stopping_song_thread = new boost::thread(stopsongtracker_thread, mainthread_callback, data);
	}
	void stopSongTrackerConcurrent()
	{
		stopSongTrackerConcurrent(NULL, NULL);
	}

	void toggleEditMode()
	{
		edit_mode = !edit_mode;
		mw->updateEditMode();
	}

	void auditionNote(int channel, int octave, int note)
	{
		int instrument = activeDocInfo()->currentInstrument();
		sgen->auditionNote(note - C, octave, instrument, channel);
	}
	void auditionNoteHalt()
	{
		sgen->auditionHalt();
	}
	void auditionDPCM(const CDSample *sample)
	{

	}
	void setMuted(int channel, bool muted)
	{
		FtmDocument_lock_guard lock(activeDocument());

		sgen->trackerController()->setMuted(channel, muted);
	}
	bool isMuted(int channel)
	{
		FtmDocument_lock_guard lock(activeDocument());

		bool muted = sgen->trackerController()->muted(channel);

		return muted;
	}
	void unmuteAll()
	{
		FtmDocument *doc = gui::activeDocument();
		FtmDocument_lock_guard lock(doc);

		int chans = doc->GetAvailableChannels();

		for (int i = 0; i < chans; i++)
		{
			sgen->trackerController()->setMuted(i, false);
		}
	}

	void toggleSolo(int channel)
	{
		// if all channels but "channel" are muted, unmute all channels
		// else, mute all channels but "channel"

		FtmDocument *doc = gui::activeDocument();
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
