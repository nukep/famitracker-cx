#ifndef _TRACKERCONTROLLER_HPP_
#define _TRACKERCONTROLLER_HPP_

#include "FamiTrackerTypes.h"
#include "common.hpp"

class FtmDocument;
class CTrackerChannel;
struct stChanNote;

class FAMICOREAPI TrackerController
{
public:
	TrackerController();
	~TrackerController();
	void tick();
	void playRow();
	void startAt(unsigned int frame, unsigned int row);
	void setFrame(unsigned int frame);
	void skip(unsigned int row);

	void setTempo(unsigned int tempo, unsigned int speed);

	void initialize(FtmDocument *doc, CTrackerChannel * const * trackerChannels);

	unsigned int frame() const{ return m_frame; }
	unsigned int row() const{ return m_row; }
	bool isHalted() const{ return m_halted; }
	FtmDocument * document() const{ return m_document; }

	void setMuted(int channel_offset, bool mute);
	bool muted(int channel_offset){ return m_muted[channel_offset]; }
private:
	void evaluateGlobalEffects(const stChanNote *noteData, int effColumns);

	FtmDocument * m_document;
	CTrackerChannel * const * m_trackerChannels;

	bool m_jumped;
	bool m_didJump;
	bool m_halted;
	unsigned int m_frame, m_row;
	unsigned int m_jumpFrame, m_jumpRow;

	unsigned int m_lastDocTempo, m_lastDocSpeed;
	unsigned int m_tempo, m_speed;
	int m_tempoAccum, m_tempoDecrement;

	unsigned int m_elapsedFrames;

	bool m_muted[MAX_CHANNELS];
};

#endif

