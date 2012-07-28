#ifndef _GUI_HPP_
#define _GUI_HPP_

#include "famitracker-core/Document.hpp"
#include "famitracker-core/FamiTrackerTypes.h"

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

	class DocInfo;
	class MainWindow;

	unsigned int loadedDocuments();
	FtmDocument * activeDocument();
	DocInfo * activeDocInfo();
	void closeActiveDocument();
	void openDocument(core::IO *io, bool close_active);
	void newDocument(bool close_active);

	bool isPlaying();
	bool isEditing();
	static bool canEdit(){ return isEditing() && (!isPlaying()); }
	void playSong();
	void stopSong();
	void sink_block();
	void stopSong();
	void stopSongConcurrent(QEvent *event);
	void stopSongConcurrent(void (*mainthread_callback)(MainWindow *, void*), void *data=NULL);
	void stopSongConcurrent();
	void stopSongTrackerConcurrent(void (*mainthread_callback)(MainWindow *, void*), void *data=NULL);
	void stopSongTrackerConcurrent();
	static inline void toggleSongPlaying()
	{
		if (isPlaying())
		{
			stopSongTrackerConcurrent();
		}
		else
		{
			playSong();
		}
	}
	void toggleEditMode();

	void auditionNote(int channel, int octave, int note);
	void auditionNoteHalt();
	void auditionDPCM(const CDSample *sample);

	void setMuted(int channel, bool muted);
	bool isMuted(int channel);
	static inline void toggleMuted(int channel)
	{
		setMuted(channel, !isMuted(channel));
	}
	void unmuteAll();
	void toggleSolo(int channel);

	class DocInfo
	{
		friend void gui::closeActiveDocument();
	public:
		DocInfo(FtmDocument *d);
		FtmDocument * doc() const{ return m_doc; }
		void setCurrentFrame(unsigned int frame);
		void setCurrentChannel(unsigned int chan);
		void setCurrentChannelColumn(unsigned int col);
		void setCurrentChannelColumn_pos(unsigned int col);
		void setCurrentRow(unsigned int row);
		void setVolumes(const core::u8 *vols);
		void setCurrentInstrument(unsigned int inst){ m_currentInstrument = inst; }
		void setCurrentOctave(unsigned int octave){ m_currentOctave = octave; }
		void scrollFrameBy(int delta);
		void scrollChannelBy(int delta);
		void scrollChannelColumnBy(int delta);
		void scrollOctaveBy(int delta);
		unsigned int currentFrame() const{ return m_currentFrame; }
		unsigned int currentChannel() const{ return m_currentChannel; }
		unsigned int currentChannelColumn() const{ return m_currentChannelColumn; }
		unsigned int currentChannelColumn_pos() const;
		unsigned int currentRow() const{ return m_currentRow; }
		const core::u8 * volumes() const{ return m_vols; }
		unsigned int currentInstrument() const{ return m_currentInstrument; }
		unsigned int currentOctave() const{ return m_currentOctave; }
		unsigned int patternColumns(unsigned int chan) const;

		void setEditStep(unsigned int step){ m_step = step; }
		void setKeyRepetition(bool repeat){ m_keyrepetition = repeat; }
		unsigned int editStep() const{ return m_step; }
		bool keyRepetition() const{ return m_keyrepetition; }

		// outputs correct notation based on settings
		// note: 0..11, out: writes 2 characters (eg. Bb or A#)
		void noteNotation(unsigned int note, char *out);
	protected:
		FtmDocument * m_doc;
		unsigned int m_currentFrame, m_currentChannel, m_currentRow;
		unsigned int m_currentChannelColumn;
		unsigned int m_currentInstrument;
		unsigned int m_currentOctave;

		unsigned int m_step;
		bool m_keyrepetition;

		bool m_notesharps;

		core::u8 m_vols[MAX_CHANNELS];

		void destroy();
	};
}

#endif
