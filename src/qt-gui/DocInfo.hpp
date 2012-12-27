#ifndef _DOCINFO_HPP_
#define _DOCINFO_HPP_

#include "core/common.hpp"
#include "famitracker-core/FamiTrackerTypes.h"
class FtmDocument;

namespace gui
{
	class App;
	class DocInfo
	{
		friend class gui::App;
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

