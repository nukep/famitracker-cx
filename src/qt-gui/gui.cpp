#include <QApplication>
#include <QFile>
#include <vector>
#include <QDebug>
#include <QThread>
#include "gui.hpp"
#include "MainWindow.hpp"
#include "threads.hpp"
#include "core/FtmDocument.hpp"
#include "core/sound.hpp"
#include "core/TrackerController.hpp"
#include "core/App.hpp"

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
	SoundGenThread *sgen_thread;
	SoundSinkPlayback *sink;

	QApplication *app;
	MainWindow *mw;

	bool edit_mode = false;

	static void trackerUpdate(SoundGen *gen)
	{
		// happens on non-gui thread
		const TrackerController *c = gen->trackerController();
		activeDocInfo()->setCurrentFrame(c->frame());
		activeDocInfo()->setCurrentRow(c->row());

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

		sink = (SoundSinkPlayback*)app::loadSoundSink(sound_name);
		sink->initialize(48000, 1, 150);
		sgen = new SoundGen;
		sgen->setSoundSink(sink);
		sgen->setTrackerUpdate(trackerUpdate);

		sgen_thread = new SoundGenThread(sgen);

		newDocument(false);

		mw = new MainWindow;
		mw->resize(1024, 768);
	}
	void destroy()
	{
		stopSong();

		delete mw;
		delete app;

		delete sgen_thread;

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

		stopSong();

		loaded_documents[active_doc_index].destroy();
		loaded_documents.erase(loaded_documents.begin()+active_doc_index);

		setActiveDocument(active_doc_index-1);
	}

	void openDocument(FileIO *io, bool close_active)
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
		return sgen_thread->isRunning();
	}
	bool isEditing()
	{
		return edit_mode;
	}

	void playSong()
	{
		if (sgen_thread->isRunning())
			return;

		if (mw != NULL)
			mw->setPlaying(true);

		const DocInfo *dinfo = activeDocInfo();

		sgen->trackerController()->startAt(dinfo->currentFrame(), 0);

		sgen_thread->run();
	}

	void stopSong()
	{
		if (sgen_thread->isRunning())
		{
			sgen->requestStop();
		}
		sgen_thread->wait();

		if (mw != NULL)
			mw->setPlaying(false);
	}

	void toggleEditMode()
	{
		edit_mode = !edit_mode;
		mw->updateEditMode();
	}

	FileIO::FileIO(const QString &name, bool reading)
	{
		f = new QFile(name);
		if (!f->open(reading ? QIODevice::ReadOnly : QIODevice::WriteOnly))
		{
			abort();
		}
	}
	FileIO::~FileIO()
	{
		delete f;
	}

	Quantity FileIO::read(void *buf, Quantity sz)
	{
		return f->read((char*)buf, sz);
	}
	Quantity FileIO::write(const void *buf, Quantity sz)
	{
		return f->write((const char*)buf, sz);
	}
	bool FileIO::seek(int offset, SeekOrigin o)
	{
		switch(o)
		{
		case IO_SEEK_SET:
			return f->seek(offset);
			break;
		case IO_SEEK_CUR:
			return f->seek(f->pos() + offset);
			break;
		case IO_SEEK_END:
			return f->seek(f->size());
			break;
		}
	}
	bool FileIO::isReadable()
	{
		return f->isReadable();
	}
	bool FileIO::isWritable()
	{
		return f->isWritable();
	}
}
