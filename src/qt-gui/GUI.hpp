#ifndef _GUI_HPP_
#define _GUI_HPP_

#include "famitracker-core/Document.hpp"
#include "famitracker-core/FamiTrackerTypes.h"
#include "DocInfo.hpp"

class QWidget;
class QFile;
class QString;
class QEvent;
class FtmDocument;

namespace gui
{
	void init(int &argc, char **argv);
	void init_2(const char *sound_name);
	void destroy();
	void spin();

	void updateFrameChannel(bool modified=false);
	void updateOctave();

	class MainWindow;

	unsigned int loadedDocuments();
	FtmDocument * activeDocument();
	DocInfo * activeDocInfo();
	void closeActiveDocument();
	void openDocument(core::IO *io, bool close_active);
	void newDocument(bool close_active);

	typedef void (*mainthread_callback_t)(MainWindow *, void*);

	bool isPlaying();
	bool isEditing();
	bool canEdit();
	void playSongConcurrent(mainthread_callback_t, void *data=NULL);
	void playSongConcurrent();
	void playSongAtRowConcurrent();
	void stopSongConcurrent(mainthread_callback_t, void *data=NULL);
	void stopSongConcurrent();
	void stopSongTrackerConcurrent(mainthread_callback_t, void *data=NULL);
	void stopSongTrackerConcurrent();
	void deleteSinkConcurrent(mainthread_callback_t cb, void *data=NULL);
	void toggleSongPlaying();
	void toggleEditMode();

	void auditionNote(int channel, int octave, int note);
	void auditionRow();
	void auditionNoteHalt();
	void auditionDPCM(const CDSample *sample);

	void setMuted(int channel, bool muted);
	bool isMuted(int channel);
	void toggleMuted(int channel);
	void unmuteAll();
	void toggleSolo(int channel);

	void promptUnimplemented(QWidget *parent);
}

#endif
