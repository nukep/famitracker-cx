#include <QApplication>
#include <QFile>
#include <vector>
#include <queue>
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

	enum
	{
		T_PLAYSONG, T_STOPSONG, T_STOPSONGTRACKER,
		T_AUDITION, T_HALTAUDITION,
		T_DELETESINK, T_TERMINATE
	};

	struct threadpool_playing_task
	{
		threadpool_playing_task(){}
		threadpool_playing_task(int task)
			: m_task(task), m_cb(NULL), m_cb_data(NULL)
		{
		}
		threadpool_playing_task(int task, mainthread_callback_t cb, void *cb_data)
			: m_task(task), m_cb(cb), m_cb_data(cb_data)
		{
		}

		int m_task;
		mainthread_callback_t m_cb;
		void * m_cb_data;

		struct audition_t
		{
			bool playrow;
			int octave, note, inst, channel;
		};
		struct playing_t
		{
			bool startatrow0;
		};

		union
		{
			playing_t m_playing;
			audition_t m_audition;
		};
	};

	boost::thread *thread_threadpool_playing;
	boost::mutex mtx_threadpool_playing;
	boost::condition cond_threadpool_playing;
	std::queue<threadpool_playing_task> threadpool_playing_queue;

	static void threadpool_playing_post(const threadpool_playing_task &t)
	{
		mtx_threadpool_playing.lock();
		threadpool_playing_queue.push(t);
		cond_threadpool_playing.notify_all();
		mtx_threadpool_playing.unlock();
	}
	static void func_threadpool_playing();

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

		if (rf.halt_signal)
		{
			stopSongTrackerConcurrent();
		}
	}

	void init(int &argc, char **argv)
	{
		app = new QApplication(argc, argv);
		app->setApplicationName(QApplication::tr("FamiTracker CX"));
		app->setWindowIcon(QIcon(":/appicon"));
	}

	void init_2(const char *sound_name)
	{
		thread_threadpool_playing = new boost::thread(func_threadpool_playing);

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
		threadpool_playing_post(threadpool_playing_task(T_TERMINATE));
		thread_threadpool_playing->join();
	//	delete sink;
		delete sgen;

		delete mw;
		delete thread_threadpool_playing;
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

	static void playsong_thread(mainthread_callback_t cb, void *data, const threadpool_playing_task::playing_t &p)
	{
		const DocInfo *dinfo = activeDocInfo();

		bool startatrow0 = p.startatrow0;

		int row = startatrow0?0:dinfo->currentRow();

		sgen->trackerController()->startAt(dinfo->currentFrame(), row);

		sgen->startTracker();

		mtx_is_playing.lock();
		is_playing = true;
		mtx_is_playing.unlock();

		if (mw != NULL)
		{
			mw->sendIsPlayingSongEvent(cb, data, true);
		}
	}

	static void stopsong_thread(mainthread_callback_t cb, void *data)
	{
		sgen->stopTracker();
		sink->blockUntilStopped();
		sink->blockUntilTimerEmpty();

		mtx_is_playing.lock();
		is_playing = false;
		mtx_is_playing.unlock();

		if (mw != NULL)
		{
			mw->sendIsPlayingSongEvent(cb, data, false);
		}
	}

	static void stopsongtracker_thread(mainthread_callback_t cb, void *data)
	{
		sgen->stopTracker();
		sgen->blockUntilTrackerStopped();

		mtx_is_playing.lock();
		is_playing = false;
		mtx_is_playing.unlock();

		if (mw != NULL)
		{
			mw->sendIsPlayingSongEvent(cb, data, false);
		}
	}

	static void auditionnote_thread(const threadpool_playing_task::audition_t &a)
	{
		if (a.playrow)
		{
			DocInfo *dinfo = activeDocInfo();
			sgen->auditionRow(dinfo->currentFrame(), dinfo->currentRow());
		}
		else
		{
			sgen->auditionNote(a.note, a.octave, a.inst, a.channel);
		}
	}

	static void auditionhalt_thread()
	{
		sgen->auditionHalt();
	}

	static void deletesink_thread(mainthread_callback_t cb, void *data)
	{
		delete sink;
		sink = NULL;

		if (mw != NULL)
		{
			mw->sendIsPlayingSongEvent(cb, data, isPlaying());
		}
	}

	static void func_threadpool_playing()
	{
		while (true)
		{
			threadpool_playing_task t;
			mtx_threadpool_playing.lock();
		checkempty:
			bool empty = threadpool_playing_queue.empty();
			if (!empty)
			{
				t = threadpool_playing_queue.front();
				threadpool_playing_queue.pop();
			}
			if (empty)
			{
				// wait until queue is filled
				cond_threadpool_playing.wait(mtx_threadpool_playing);
				goto checkempty;
			}
			mtx_threadpool_playing.unlock();

			// do what's on the queue

			switch (t.m_task)
			{
			case T_PLAYSONG:
				playsong_thread(t.m_cb, t.m_cb_data, t.m_playing);
				break;
			case T_STOPSONG:
				stopsong_thread(t.m_cb, t.m_cb_data);
				break;
			case T_STOPSONGTRACKER:
				stopsongtracker_thread(t.m_cb, t.m_cb_data);
				break;
			case T_AUDITION:
				auditionnote_thread(t.m_audition);
				break;
			case T_HALTAUDITION:
				auditionhalt_thread();
				break;

			case T_DELETESINK:
				deletesink_thread(t.m_cb, t.m_cb_data);
				break;
			// gracefully end the thread pool
			case T_TERMINATE:
				return;

			default:	// ignore
				break;
			}
		}
	}

	void playSongConcurrent(mainthread_callback_t cb, void *data)
	{
		threadpool_playing_task t(T_PLAYSONG, cb, data);
		t.m_playing.startatrow0 = true;
		threadpool_playing_post(t);
	}

	void playSongConcurrent()
	{
		playSongConcurrent(NULL, NULL);
	}

	void playSongAtRowConcurrent()
	{
		threadpool_playing_task t(T_PLAYSONG);
		t.m_playing.startatrow0 = false;
		threadpool_playing_post(t);
	}

	void stopSongConcurrent(mainthread_callback_t cb, void *data)
	{
		threadpool_playing_task t(T_STOPSONG, cb, data);
		threadpool_playing_post(t);
	}
	void stopSongConcurrent()
	{
		stopSongConcurrent(NULL, NULL);
	}

	void stopSongTrackerConcurrent(mainthread_callback_t cb, void *data)
	{
		threadpool_playing_task t(T_STOPSONGTRACKER, cb, data);
		threadpool_playing_post(t);
	}
	void stopSongTrackerConcurrent()
	{
		stopSongTrackerConcurrent(NULL, NULL);
	}
	void deleteSinkConcurrent(mainthread_callback_t cb, void *data)
	{
		threadpool_playing_task t(T_DELETESINK, cb, data);
		threadpool_playing_post(t);
	}

	void toggleEditMode()
	{
		edit_mode = !edit_mode;
		mw->updateEditMode();
	}

	void auditionNote(int channel, int octave, int note)
	{
		threadpool_playing_task t(T_AUDITION);
		t.m_audition.playrow = false;
		t.m_audition.octave = octave;
		t.m_audition.note = note - C;
		t.m_audition.inst = activeDocInfo()->currentInstrument();
		t.m_audition.channel = channel;
		threadpool_playing_post(t);
	}
	void auditionRow()
	{
		threadpool_playing_task t(T_AUDITION);
		t.m_audition.playrow = true;
		threadpool_playing_post(t);
	}
	void auditionNoteHalt()
	{
		threadpool_playing_task t(T_HALTAUDITION);
		threadpool_playing_post(t);
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
