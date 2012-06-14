#ifndef _GUI_HPP_
#define _GUI_HPP_

#include "core/Document.hpp"
#include "core/FamiTrackerTypes.h"

class QFile;
class QString;
class FtmDocument;

namespace gui
{
	void init(int &argc, char **argv);
	void init_2(const char *sound_name);
	void destroy();
	void spin();

	void updateFrameChannel(bool modified=false);

	class DocInfo;

	unsigned int loadedDocuments();
	FtmDocument * activeDocument();
	DocInfo * activeDocInfo();
	void closeActiveDocument();
	class FileIO;
	void openDocument(FileIO *io, bool close_active);
	void newDocument(bool close_active);

	bool isPlaying();
	bool isEditing();
	static bool canEdit(){ return isEditing() && (!isPlaying()); }
	void playSong();
	void stopSong();
	static inline void toggleSong()
	{
		if (isPlaying())
		{
			stopSong();
		}
		else
		{
			playSong();
		}
	}
	void toggleEditMode();

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
		void setCurrentInstrument(unsigned int inst){ m_currentInstrument = inst; }
		void scrollFrameBy(int delta);
		void scrollChannelBy(int delta);
		void scrollChannelColumnBy(int delta);
		unsigned int currentFrame() const{ return m_currentFrame; }
		unsigned int currentChannel() const{ return m_currentChannel; }
		unsigned int currentChannelColumn() const{ return m_currentChannelColumn; }
		unsigned int currentChannelColumn_pos() const;
		unsigned int currentRow() const{ return m_currentRow; }
		unsigned int currentInstrument() const{ return m_currentInstrument; }
		unsigned int framePlayLength(unsigned int frame) const;
		unsigned int patternColumns(unsigned int chan) const;
		void calculateFramePlayLengths();
	protected:
		FtmDocument * m_doc;
		unsigned int m_currentFrame, m_currentChannel, m_currentRow;
		unsigned int m_currentChannelColumn;
		unsigned int m_currentInstrument;
		unsigned int m_framePlayLengths[MAX_FRAMES];

		void destroy();
	};

	class FileIO : public IO
	{
	public:
		FileIO(const QString &name, bool reading);
		~FileIO();
		Quantity read(void *buf, Quantity sz);
		Quantity write(const void *buf, Quantity sz);
		bool seek(int offset, SeekOrigin o);
		bool isReadable();
		bool isWritable();
	private:
		QFile *f;
	};
}

#endif
