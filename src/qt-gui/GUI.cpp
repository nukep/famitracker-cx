#include <QApplication>
#include <QFile>
#include <QIcon>
#include <QDebug>
#include "GUI.hpp"
#include "GUI_App.hpp"

namespace gui
{
	static App *app;

	// let Qt evaluate the command line arguments
	void init(int &argc, char **argv)
	{
		QApplication *a = new QApplication(argc, argv);
		a->setApplicationName(QApplication::tr("FamiTracker CX"));
		a->setWindowIcon(QIcon(":/appicon"));

		app = new App(a);
	}

	// after Qt has had a chance to handle the command line arguments
	void init_2(const char *sound_name)
	{
		app->init2(sound_name);
	}
	void destroy()
	{
		app->destroy();
		delete app;
	}
	void spin()
	{
		app->spin();
	}

	void updateFrameChannel(bool modified)
	{
		app->updateFrameChannel(modified);
	}
	void updateOctave()
	{
		app->updateOctave();
	}

	unsigned int loadedDocuments()
	{
		return app->loadedDocuments();
	}
	FtmDocument * activeDocument()
	{
		return app->activeDocument();
	}
	DocInfo * activeDocInfo()
	{
		return app->activeDocInfo();
	}

	void closeActiveDocument()
	{
		app->closeActiveDocument();
	}

	void openDocument(core::IO *io, bool close_active)
	{
		app->openDocument(io, close_active);
	}
	void newDocument(bool close_active)
	{
		app->newDocument(close_active);
	}

	bool isPlaying()
	{
		return app->isPlaying();
	}
	bool isEditing()
	{
		return app->isEditing();
	}
	bool canEdit()
	{
		return app->canEdit();
	}

	void playSongConcurrent(mainthread_callback_t cb, void *data)
	{
		app->playSongConcurrent(cb, data);
	}

	void playSongConcurrent()
	{
		app->playSongConcurrent();
	}

	void playSongAtRowConcurrent()
	{
		app->playSongAtRowConcurrent();
	}

	void stopSongConcurrent(mainthread_callback_t cb, void *data)
	{
		app->stopSongConcurrent(cb, data);
	}
	void stopSongConcurrent()
	{
		app->stopSongConcurrent();
	}

	void stopSongTrackerConcurrent(mainthread_callback_t cb, void *data)
	{
		app->stopSongTrackerConcurrent(cb, data);
	}
	void stopSongTrackerConcurrent()
	{
		app->stopSongTrackerConcurrent();
	}
	void deleteSinkConcurrent(mainthread_callback_t cb, void *data)
	{
		app->deleteSinkConcurrent(cb, data);
	}

	void toggleSongPlaying()
	{
		app->toggleSongPlaying();
	}

	void toggleEditMode()
	{
		app->toggleEditMode();
	}

	void auditionNote(int channel, int octave, int note)
	{
		app->auditionNote(channel, octave, note);
	}
	void auditionRow()
	{
		app->auditionRow();
	}
	void auditionNoteHalt()
	{
		app->auditionNoteHalt();
	}
	void auditionDPCM(const CDSample *sample)
	{
		app->auditionDPCM(sample);
	}
	void setMuted(int channel, bool muted)
	{
		app->setMuted(channel, muted);
	}
	bool isMuted(int channel)
	{
		return app->isMuted(channel);
	}
	void toggleMuted(int channel)
	{
		app->toggleMuted(channel);
	}

	void unmuteAll()
	{
		app->unmuteAll();
	}

	void toggleSolo(int channel)
	{
		app->toggleSolo(channel);
	}
}
