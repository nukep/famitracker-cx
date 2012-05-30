#ifndef _SOUND_HPP_
#define _SOUND_HPP_

#include "../APU/APU.H"

class SoundSink : public ICallback
{
};

class CDSample;
struct stChanNote;
class CTrackerChannel;
class CChannelHandler;

const int VIBRATO_LENGTH = 256;
const int TREMOLO_LENGTH = 256;

const int NOTE_COUNT = 96;	// 96 available notes

typedef enum { SONG_TIME_LIMIT, SONG_LOOP_LIMIT } RENDER_END;

class SoundGen
{
public:
	SoundGen();
	~SoundGen();

	void setSoundSink(SoundSink *s);

	// TODO - dan (dan's implementation)
	bool isPlaying() const{ return false; }

	// Vibrato
	void generateVibratoTable(int type);
	int readVibratoTable(int index) const{ return m_iVibratoTable[index]; }

	// Tracker playing
	void evaluateGlobalEffects(stChanNote *noteData, int effColumns);

	// Rendering
	void checkRenderStop();
	void frameIsDone(int skipFrames);

	// Used by channels
	void addCycles(int count);

private:
	// Internal initialization
	void createChannels();
	void assignChannel(CTrackerChannel *trackerChannel, CChannelHandler *renderer);
	void resetAPU();

	// Player
	void playNote(int channel, stChanNote *noteData, int effColumns);

	// Misc
	void playSample(CDSample *sample, int offset, int pitch);

private:
	// Objects
	CChannelHandler * m_pChannels[CHANNELS];
	CTrackerChannel * m_pTrackerChannels[CHANNELS];

	// Sound
	CSampleMem m_samplemem;
	CAPU m_apu;
	SoundSink *m_sink;

// Tracker playing variables
private:
	unsigned int		m_iTempo, m_iSpeed;					// Tempo and speed
	int					m_iTempoAccum;						// Used for speed calculation
	unsigned int		m_iPlayTime;
	volatile bool		m_bPlaying;
	bool				m_bPlayLooping;
	int					m_iFrameCounter;

	int					m_iUpdateCycles;					// Number of cycles/APU update

	int					m_iConsumedCycles;					// Cycles consumed by the update registers functions

	// Play control
	int					m_iJumpToPattern;
	int					m_iSkipToRow;
	int					m_iStepRows;
	int					m_iPlayMode;

	unsigned int		*m_pNoteLookupTable;				// NTSC or PAL
	unsigned int		m_iNoteLookupTableNTSC[96];			// For 2A03
	unsigned int		m_iNoteLookupTablePAL[96];			// For 2A07
	unsigned int		m_iNoteLookupTableSaw[96];			// For VRC6 sawtooth
	unsigned int		m_iNoteLookupTableFDS[96];			// For FDS
	unsigned int		m_iNoteLookupTableN106[96];			// For N106
	unsigned int		m_iNoteLookupTableS5B[96];			// For sunsoft
	int					m_iVibratoTable[VIBRATO_LENGTH];

	unsigned int		m_iMachineType;						// NTSC/PAL
	bool				m_bRunning;							// Main loop is running

	// Rendering
	RENDER_END			m_iRenderEndWhen;
	int					m_iRenderEndParam;
	int					m_iRenderedFrames;
	int					m_iRenderedSong;
	int					m_iDelayedStart;
	int					m_iDelayedEnd;

	int					m_iTempoDecrement;
	bool				m_bUpdateRow;

	bool				m_bRendering;
	bool				m_bRequestRenderStop;
	bool				m_bPlayerHalted;
};

#endif

