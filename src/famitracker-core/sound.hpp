#ifndef _SOUND_HPP_
#define _SOUND_HPP_

#include "APU/APU.h"
#include "core/soundsink.hpp"
#include "core/ringbuffer.hpp"

class CDSample;
struct stChanNote;
class CTrackerChannel;
class CChannelHandler;
class FtmDocument;
class TrackerController;

const int VIBRATO_LENGTH = 256;
const int TREMOLO_LENGTH = 256;

const int NOTE_COUNT = 96;	// 96 available notes

typedef enum { SONG_TIME_LIMIT, SONG_LOOP_LIMIT } RENDER_END;

class SoundGen
{
public:
	typedef void (*trackerupdate_f)(SoundGen*);
	SoundGen();
	~SoundGen();

	void setSoundSink(core::SoundSink *s);

	bool isRunning() const{ return m_bRunning; }

	void setDocument(FtmDocument *doc);
	TrackerController * trackerController() const{ return m_trackerctlr; }
	void setTrackerUpdate(trackerupdate_f f){ m_trackerUpdateCallback = f; }

	// Multiple times initialization
	void loadMachineSettings(int machine, int rate);

	// Vibrato
	void generateVibratoTable(int type);
	int readVibratoTable(int index) const{ return m_iVibratoTable[index]; }

	// Player interface
	void resetTempo();

	// Rendering
	void checkRenderStop();
	void frameIsDone(int skipFrames);

	// Used by channels
	void addCycles(int count);

	void run();

	void requestStop();

private:
	void requestFrame();
	core::u32 requestSound(core::s16 *buf, core::u32 sz, core::u32 *idx);
	void timeCallback(core::u32 skip, void *data);
	struct rowframe_t
	{
		unsigned int row, frame;
	};

	typedef core::RingBuffer<core::s16, 16384> SoundRingBuffer;

	core::RingBuffer<rowframe_t, 64> m_queued_rowframes;
	SoundRingBuffer *m_queued_sound;
	// Internal initialization
	void createChannels();
	void setupChannels();
	void assignChannel(CTrackerChannel *trackerChannel, CChannelHandler *renderer);
	void resetAPU();

	// Player
	void playNote(int channel, stChanNote *noteData, int effColumns);
	void runFrame();

	// Misc
	void playSample(CDSample *sample, int offset, int pitch);

private:
	FtmDocument *m_pDocument;
	TrackerController *m_trackerctlr;
	trackerupdate_f m_trackerUpdateCallback;
	// Objects
	CChannelHandler * m_pChannels[CHANNELS];
	CTrackerChannel * m_pTrackerChannels[CHANNELS];
	CTrackerChannel * m_pActiveTrackerChannels[CHANNELS];
private:

	// Sound
	CSampleMem m_samplemem;
	CAPU m_apu;
	core::SoundSink *m_sink;

	unsigned int m_iChannels;

// Tracker playing variables
private:
	unsigned int		m_lastRow, m_lastFrame;
	unsigned int		m_iPlayTime;
	int					m_iFrameCounter;

	int					m_iUpdateCycles;					// Number of cycles/APU update

	int					m_iConsumedCycles;					// Cycles consumed by the update registers functions

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

	bool				m_bRendering;
	volatile bool		m_bRequestStop;
	bool				m_bPlayerHalted;
};

#endif

