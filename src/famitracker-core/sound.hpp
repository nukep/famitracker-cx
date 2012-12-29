#ifndef _SOUND_HPP_
#define _SOUND_HPP_

#include "APU/APU.h"
#include "core/soundsink.hpp"
#include "FamiTrackerTypes.h"
#include "common.hpp"

namespace core
{
	class RingBuffer;
}

class CAPU;
class CSampleMem;
class CDSample;
struct stChanNote;
class CTrackerChannel;
class CChannelHandler;
class FtmDocument;
class TrackerController;

const int VIBRATO_LENGTH = 256;
const int TREMOLO_LENGTH = 256;

const int NOTE_COUNT = 12*8;	// 96 available notes

typedef enum { SONG_TIME_LIMIT, SONG_LOOP_LIMIT } RENDER_END;

struct _soundgen_threading_t;

class FAMICOREAPI SoundGen
{
public:
	struct rowframe_t
	{
		unsigned int row, frame;
		bool rowframe_changed;
		bool tracker_running;
		bool halt_signal;
		const core::u8 * volumes;
	};
	typedef void (*trackerupdate_f)(rowframe_t rf, FtmDocument *doc, void *data);
	SoundGen();
	~SoundGen();

	void setSoundSink(core::SoundSink *s);

	void setDocument(FtmDocument *doc);
	TrackerController * trackerController() const{ return m_trackerctlr; }
	void setTrackerUpdate(trackerupdate_f f, void *data=NULL){ m_trackerUpdateCallback = f; m_trackerUpdateData = data; }

	// Multiple times initialization
	void loadMachineSettings(int machine, int rate);

	// Vibrato
	void generateVibratoTable(int type);
	int readVibratoTable(int index) const{ return m_iVibratoTable[index]; }

	// Player interface
	void resetTempo();

	// Used by channels
	void addCycles(int count);

	void startTracker();
	void stopTracker();
	void auditionNote(int note, int octave, int instrument, int channel);
	void auditionRow(unsigned int frame, unsigned int row);
	void auditionHalt();

	bool isTrackerActive();
	void blockUntilTrackerStopped();

private:
	static void apuCallback(const int16 *buf, uint32 sz, void *data);
	static core::u32 soundCallback(core::s16 *buf, core::u32 sz, void *data, core::u32 *idx);
	static void timeCallback(core::u32 skip, void *data);

	void startPlayback();
	void stopPlayback();
	void haltSounds();
	void requestFrame();
	// requestSound is not guaranteed to be (and typically isn't) called at a constant rate.
	// for example, just because the engine speed may be 60Hz doesn't mean this gets called at 60Hz.
	core::u32 requestSound(core::s16 *buf, core::u32 sz, core::u32 *idx);

	core::RingBuffer *m_queued_rowframes;
	core::RingBuffer *m_queued_sound;
	core::u8 * m_volumes_ring;
	unsigned int m_volumes_read_offset, m_volumes_write_offset;
	unsigned int m_volumes_size;

	const core::u8 * readVolume();
	const core::u8 * writeVolume(const core::u8 *arr);

	// Internal initialization
	void createChannels();
	void setupChannels();
	void assignChannel(int id, CChannelHandler *renderer);
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
	void *m_trackerUpdateData;
	// Objects
	CChannelHandler * m_pChannels[CHANNELS];
	CTrackerChannel * m_pTrackerChannels[CHANNELS];
	CTrackerChannel * m_pActiveTrackerChannels[CHANNELS];
private:

	// Sound
	CSampleMem *m_samplemem;
	CAPU *m_apu;
	core::SoundSink *m_sink;

	unsigned int m_iChannels;

// Tracker playing variables
private:
	_soundgen_threading_t * m_threading;
	bool m_trackerActive;
	int m_sinkStopSamples;
	bool m_timer_trackerActive;

	unsigned int		m_lastRow, m_lastFrame;
	unsigned int		m_iPlayTime;
	int					m_iFrameCounter;

	unsigned int		m_channels;

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

	// Rendering
	RENDER_END			m_iRenderEndWhen;
	int					m_iRenderEndParam;

	bool				m_bPlayerHalted;
};

#endif

