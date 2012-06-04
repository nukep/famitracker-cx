#ifndef _TRACKERCONTROLLER_HPP_
#define _TRACKERCONTROLLER_HPP_

class FtmDocument;
class CTrackerChannel;
struct stChanNote;

class TrackerController
{
public:
	TrackerController();
	~TrackerController();
	void tick();
	void playRow();
	void setFrame(unsigned int frame);
	void skip(unsigned int row);

	void setTempo(unsigned int tempo, unsigned int speed);

	// TODO - dan: make private
	FtmDocument * m_document;
	CTrackerChannel ** m_trackerChannels;
private:
	void evaluateGlobalEffects(stChanNote *noteData, int effColumns);

	bool m_jumped;
	bool m_halted;
	unsigned int m_frame, m_row;

	unsigned int m_tempo, m_speed;
	int m_tempoAccum, m_tempoDecrement;

	unsigned int m_elapsedFrames;
};

#endif

