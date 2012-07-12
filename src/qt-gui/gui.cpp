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
		  m_currentChannelColumn(0), m_currentInstrument(0)
	{
	}
	void DocInfo::destroy()
	{
		delete m_doc;
	}
	void DocInfo::setCurrentFrame(unsigned int frame)
	{
		if (frame >= doc()->GetFrameCount())
			frame = doc()->GetFrameCount()-1;

		m_currentFrame = frame;
		setCurrentRow(m_currentRow);
	}
	void DocInfo::setCurrentChannel(unsigned int chan)
	{
		if (chan >= doc()->GetAvailableChannels())
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
		int c = col;
		for (unsigned int i = 0; i < doc()->GetAvailableChannels(); i++)
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
		if (row >= doc()->getFramePlayLength(m_currentFrame))
			row = doc()->getFramePlayLength(m_currentFrame)-1;
		m_currentRow = row;
	}
	void DocInfo::scrollFrameBy(int delta)
	{
		int f = currentFrame();
		int r = currentRow();
		r += delta;

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

		setCurrentFrame(f);
		setCurrentRow(r);
	}
	void DocInfo::scrollChannelBy(int delta)
	{
		int ch = m_currentChannel + delta;
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
		for (unsigned int i = 0; i < doc()->GetAvailableChannels(); i++)
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
		int effcol = doc()->GetEffColumns(chan);
		return C_EFF_NUM + C_EFF_COL_COUNT*(effcol+1);
	}

	typedef std::vector<DocInfo> DocsList;
	DocsList loaded_documents;
	int active_doc_index;
	SoundGen *sgen;
	core::SoundSinkPlayback *sink;

	QApplication *app;
	MainWindow *mw;

	bool edit_mode = false;

	static void trackerUpdate(SoundGen::rowframe_t rf, FtmDocument *doc)
	{
		// happens on non-gui thread

		activeDocInfo()->setCurrentFrame(rf.frame);
		activeDocInfo()->setCurrentRow(rf.row);

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
		delete mw;
		delete app;

		delete sgen;
		delete sink;
	}
	void spin()
	{
		mw->show();
		app->exec();
	}

	void updateFrameChannel(bool modified)
	{
		mw->updateFrameChannel(modified);
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
	}

	bool isPlaying()
	{
		return sink->isPlaying();
	}
	bool isEditing()
	{
		return edit_mode;
	}

	volatile bool stopping_song = false;
	boost::thread *stopping_song_thread = NULL;

	void playSong()
	{
		if (stopping_song)
			return;

		if (mw != NULL)
			mw->setPlaying(true);

		const DocInfo *dinfo = activeDocInfo();

		sgen->trackerController()->startAt(dinfo->currentFrame(), 0);

		sgen->start();
	}

	void stopSong_block()
	{
		sgen->stop();

		if (mw != NULL)
			mw->setPlaying(false);
	}

	static void stopsong_thread(void (*mainthread_callback)(MainWindow *, void*), void *data)
	{
		sgen->stop();
		sink->blockUntilStopped();
		if (mw != NULL)
		{
			mw->sendStoppedSongEvent(mainthread_callback, data);
		}
		stopping_song = false;
	}
	static void stopsong_qevent_thread(QEvent *e)
	{
		sgen->stop();
		sink->blockUntilStopped();
		QApplication::postEvent(mw, e);
		stopping_song = false;
	}

	void stopSongConcurrent(QEvent *event)
	{
		ftkr_Assert(stopping_song == false);

		if (stopping_song_thread != NULL)
			delete stopping_song_thread;

		stopping_song = true;
		stopping_song_thread = new boost::thread(stopsong_qevent_thread, event);
	}
	void stopSongConcurrent(void (*mainthread_callback)(MainWindow *, void*), void *data)
	{
		// stop the song without deadlocking the main thread
		// should only be called from the main thread

		ftkr_Assert(stopping_song == false);

		if (stopping_song_thread != NULL)
			delete stopping_song_thread;

		stopping_song = true;
		stopping_song_thread = new boost::thread(stopsong_thread, mainthread_callback, data);
	}
	void stopSongConcurrent()
	{
		if (stopping_song)
			return;

		stopSongConcurrent(NULL, NULL);
	}

	void toggleEditMode()
	{
		edit_mode = !edit_mode;
		mw->updateEditMode();
	}
}
