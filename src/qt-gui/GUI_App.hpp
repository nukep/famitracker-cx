#ifndef _GUI_APP_HPP_
#define _GUI_APP_HPP_

#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>
#include <boost/thread/condition.hpp>
#include <vector>
#include "DocInfo.hpp"
#include "famitracker-core/SoundGen.hpp"
#include "core/soundsink.hpp"
#include "core/threadpool.hpp"

class QApplication;

namespace boost
{
	class thread;
}

namespace gui
{
	class MainWindow;
	class ThreadPool;

	typedef void (*mainthread_callback_t)(MainWindow *, void*);

	class App
	{
		friend class ThreadPool;
	public:
		App(QApplication *a);
		void init2(const char *sndname);
		void destroy();
		void spin();

		void updateFrameChannel(bool modified=false);
		void updateOctave();

		unsigned int loadedDocuments();
		FtmDocument * activeDocument();
		DocInfo * activeDocInfo();
		void closeActiveDocument();
		bool openDocument(core::IO *io, bool close_active);
		void newDocument(bool close_active);
		void reloadAudio();

		bool isPlaying();
		bool isEditing();
		bool canEdit(){ return isEditing() && (!isPlaying()); }

		void setIsPlaying(bool playing);

		void sendUpdateEvent();
		void sendCallbackEvent(mainthread_callback_t cb, void *data);

		void playSongConcurrent(mainthread_callback_t, void *data=NULL);
		void playSongConcurrent(){ playSongConcurrent(NULL, NULL); }

		void playSongAtRowConcurrent();

		void stopSongConcurrent(mainthread_callback_t, void *data=NULL);
		void stopSongConcurrent(){ stopSongConcurrent(NULL, NULL); }

		void stopSongTrackerConcurrent(mainthread_callback_t, void *data=NULL);
		void stopSongTrackerConcurrent(){ stopSongTrackerConcurrent(NULL, NULL); }

		void deleteSinkConcurrent(mainthread_callback_t cb, void *data=NULL);

		void toggleSongPlaying()
		{
			if (isPlaying())
			{
				stopSongTrackerConcurrent();
			}
			else
			{
				playSongConcurrent();
			}
		}
		void toggleEditMode();

		void auditionNote(int channel, int octave, int note);
		void auditionRow();
		void auditionNoteHalt();
		void auditionDPCM(const CDSample *sample);

		void setMuted(int channel, bool muted);
		bool isMuted(int channel);
		void toggleMuted(int channel)
		{
			setMuted(channel, !isMuted(channel));
		}
		void unmuteAll();
		void toggleSolo(int channel);

		QApplication * qtApp() const{ return app; }

	private:
		void setActiveDocument(int idx);
		static void trackerUpdate_bootstrap(SoundGen::rowframe_t rf, FtmDocument *doc, void *data);
		void trackerUpdate(const SoundGen::rowframe_t &rf, FtmDocument *doc);

		typedef std::vector<DocInfo> DocsList;
		DocsList loaded_documents;
		int active_doc_index;

		SoundGen *sgen;
		core::SoundSinkPlayback *sink;
		MainWindow *mw;
		bool is_playing;
		boost::mutex mtx_is_playing;

		QApplication *app;

		bool edit_mode;

		ThreadPool *threadPool;

		boost::mutex m_mtx_updateEvent;
		boost::condition m_cond_updateEvent;
	};
}

#endif

