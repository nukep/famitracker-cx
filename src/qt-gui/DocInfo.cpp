#include <string.h>
#include "famitracker-core/FtmDocument.hpp"
#include "DocInfo.hpp"

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

		unsigned int patcols = patternColumns(chan);
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
		else if ((unsigned int)r >= doc()->getFramePlayLength(f))
		{
			r = r - doc()->getFramePlayLength(f);
			f++;
			if ((unsigned int)f >= doc()->GetFrameCount())
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
		while ((unsigned int)ch >= doc()->GetAvailableChannels())
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
		if (delta < 0 && (-delta > (int)m_currentOctave))
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
}
