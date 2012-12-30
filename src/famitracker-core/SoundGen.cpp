#include <stdlib.h>
#include <stdio.h>
#include <cmath>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>
#include "SoundGen.hpp"
#include "core/ringbuffer.hpp"
#include "FtmDocument.hpp"
#include "FamiTrackerTypes.h"
#include "TrackerChannel.h"
#include "TrackerController.hpp"

#include "ChannelHandler.h"
#include "Channels2A03.h"
#include "ChannelsFDS.h"
#include "ChannelsMMC5.h"
#include "ChannelsVRC6.h"
#include "ChannelsVRC7.h"

#include "App.hpp"
#include "core/time.hpp"

// The depth of each vibrato level
static const double NEW_VIBRATO_DEPTH[] = {
	1.0, 1.5, 2.5, 4.0, 5.0, 7.0, 10.0, 12.0, 14.0, 17.0, 22.0, 30.0, 44.0, 64.0, 96.0, 128.0
};

static const double OLD_VIBRATO_DEPTH[] = {
	1.0, 1.0, 2.0, 3.0, 4.0, 7.0, 8.0, 15.0, 16.0, 31.0, 32.0, 63.0, 64.0, 127.0, 128.0, 255.0
};

static const int rowframes_size = 60*8;

struct _soundgen_threading_t
{
	boost::mutex mtx_running;
	boost::mutex mtx_sink;
	boost::mutex mtx_tracker;
	boost::mutex mtx_rowframes;
	boost::condition cond_trackerhalt;
};

SoundGen::SoundGen()
	: m_iConsumedCycles(0), m_pDocument(NULL),
	  m_trackerUpdateCallback(NULL), m_sink(NULL),
	  m_volumes_ring(NULL),
	  m_trackerActive(false),
	  m_timer_trackerActive(false),
	  m_iMachineType(NTSC)
{
	m_samplemem = new CSampleMem;
	m_apu = new CAPU(m_samplemem);
	m_queued_rowframes = new core::RingBuffer(sizeof(rowframe_t));
	m_queued_sound = new core::RingBuffer(sizeof(core::s16));
	m_threading = new _soundgen_threading_t;
	// Create all kinds of channels
	createChannels();

	m_trackerctlr = new TrackerController;

	m_queued_rowframes->resize(rowframes_size);
	m_queued_sound->resize(16384);
	m_apu->SetCallback(apuCallback, this);
}

SoundGen::~SoundGen()
{
	if (m_volumes_ring != NULL)
		delete[] m_volumes_ring;

	delete m_trackerctlr;

	// Remove channels
	for (int i=0; i < CHANNELS; i++)
	{
		if (m_pChannels[i] != NULL)
			delete m_pChannels[i];

		if (m_pTrackerChannels[i] != NULL)
			delete m_pTrackerChannels[i];
	}
	delete m_threading;
	delete m_queued_sound;
	delete m_queued_rowframes;
	delete m_apu;
	delete m_samplemem;
}

void SoundGen::setSoundSink(core::SoundSink *s)
{
	if (m_sink != NULL)
	{
		m_sink->setPlaying(false);
		m_sink->blockUntilTimerEmpty();
	}
	m_queued_sound->clear();
	m_queued_rowframes->clear();
	m_sink = s;
	m_sink->setCallbackData(this);
	m_sink->setSoundCallback(soundCallback);
	m_sink->setTimeCallback(timeCallback);
}

void SoundGen::setDocument(FtmDocument *doc)
{
	m_threading->mtx_running.lock();

	m_pDocument = doc;

	generateVibratoTable(doc->GetVibratoStyle());

	// TODO - dan: load settings
	m_apu->SetupSound(m_sink->sampleRate(), 1, doc->GetMachine());
	m_apu->SetupMixer(16, 12000, 24, 100);

	loadMachineSettings(doc->GetMachine(), doc->GetEngineSpeed());

	unsigned char chip = doc->GetExpansionChip();
	m_apu->SetExternalSound(chip);
	resetAPU();

	resetTempo();

	const std::vector<int> & chans = doc->getChannelsFromChip();
	m_iChannels = chans.size();
	for (int i = 0; i < m_iChannels; i++)
	{
		m_pActiveTrackerChannels[i] = m_pTrackerChannels[chans[i]];
	}

	m_trackerctlr->initialize(doc, m_pActiveTrackerChannels);

	setupChannels();

	m_threading->mtx_running.unlock();
}

void SoundGen::loadMachineSettings(int machine, int rate)
{
	// Setup machine-type and speed
	//
	// Machine = NTSC or PAL
	//
	// Rate = frame rate (0 means machine default)
	//

	const double BASE_FREQ = 32.7032;
	double freq;
	double pitch;

	int BaseFreq	= (machine == NTSC) ? CAPU::BASE_FREQ_NTSC  : CAPU::BASE_FREQ_PAL;
	int DefaultRate = (machine == NTSC) ? CAPU::FRAME_RATE_NTSC : CAPU::FRAME_RATE_PAL;

	m_iMachineType = machine;

	m_apu->ChangeMachine(machine == NTSC ? MACHINE_NTSC : MACHINE_PAL);

	// Choose a default rate if not predefined
	if (rate == 0)
		rate = DefaultRate;

	double clock_ntsc = CAPU::BASE_FREQ_NTSC / 16.0;
	double clock_pal = CAPU::BASE_FREQ_PAL / 16.0;

	for (int i = 0; i < NOTE_COUNT; i++)
	{
		// Frequency (in Hz)
		freq = BASE_FREQ * pow(2.0, double(i) / 12.0);

		// 2A07
		pitch = (clock_pal / freq) - 0.5;
		m_iNoteLookupTablePAL[i] = (unsigned int)pitch;

		// 2A03 / MMC5 / VRC6
		pitch = (clock_ntsc / freq) - 0.5;
		m_iNoteLookupTableNTSC[i] = (unsigned int)pitch;

		// VRC6 Saw
		pitch = ((clock_ntsc * 16.0) / (freq * 14.0)) - 0.5;
		m_iNoteLookupTableSaw[i] = (unsigned int)pitch;

		// FDS
#ifdef TRANSPOSE_FDS
		Pitch = (Freq * 65536.0) / (clock_ntsc / 2.0) + 0.5;
#else
		pitch = (freq * 65536.0) / (clock_ntsc / 4.0) + 0.5;
#endif
		m_iNoteLookupTableFDS[i] = (unsigned int)pitch;
		// N106
		int C = 7; // channels
		int L = 0; // waveform length

//		Pitch = (3019898880.0 * Freq) / 21477272.7272;
		pitch = (1509949440.0 * freq) / 21477272.7272;

		//Pitch = (double(0x40000 * 45 * (C + 1) * (8 - L) * 4) * Freq) / (21477272.7272);
		//Pitch = (double(0x40000 * 45 * 8 * 8 * 4) * Freq) / (21477272.7272);

//		Period = (uint32)(((((ChannelsActive + 1) * 45 * 0x40000) / (float)21477270) * (float)CAPU::BASE_FREQ_NTSC) / m_iFrequency);
		//Pitch =
		m_iNoteLookupTableN106[i] = (unsigned int)pitch;
		// Sunsoft 5B
		pitch = (clock_ntsc / freq) - 0.5;
		m_iNoteLookupTableS5B[i] = (unsigned int)pitch;
	}

	if (machine == NTSC)
		m_pNoteLookupTable = m_iNoteLookupTableNTSC;
	else
		m_pNoteLookupTable = m_iNoteLookupTablePAL;

	// Number of cycles between each APU update
	m_iUpdateCycles = BaseFreq / rate;

	// Setup note tables
	m_pChannels[CHANID_SQUARE1]->SetNoteTable(m_pNoteLookupTable);
	m_pChannels[CHANID_SQUARE2]->SetNoteTable(m_pNoteLookupTable);
	m_pChannels[CHANID_TRIANGLE]->SetNoteTable(m_pNoteLookupTable);

	// TODO - dan
	m_pChannels[CHANID_VRC6_PULSE1]->SetNoteTable(m_iNoteLookupTableNTSC);
	m_pChannels[CHANID_VRC6_PULSE2]->SetNoteTable(m_iNoteLookupTableNTSC);
	m_pChannels[CHANID_VRC6_SAWTOOTH]->SetNoteTable(m_iNoteLookupTableSaw);
	m_pChannels[CHANID_MMC5_SQUARE1]->SetNoteTable(m_iNoteLookupTableNTSC);
	m_pChannels[CHANID_MMC5_SQUARE2]->SetNoteTable(m_iNoteLookupTableNTSC);
	m_pChannels[CHANID_FDS]->SetNoteTable(m_iNoteLookupTableFDS);

/*	m_pChannels[CHANID_N106_CHAN1]->SetNoteTable(m_iNoteLookupTableN106);
	m_pChannels[CHANID_N106_CHAN2]->SetNoteTable(m_iNoteLookupTableN106);
	m_pChannels[CHANID_N106_CHAN3]->SetNoteTable(m_iNoteLookupTableN106);
	m_pChannels[CHANID_N106_CHAN4]->SetNoteTable(m_iNoteLookupTableN106);
	m_pChannels[CHANID_N106_CHAN5]->SetNoteTable(m_iNoteLookupTableN106);
	m_pChannels[CHANID_N106_CHAN6]->SetNoteTable(m_iNoteLookupTableN106);
	m_pChannels[CHANID_N106_CHAN7]->SetNoteTable(m_iNoteLookupTableN106);
	m_pChannels[CHANID_N106_CHAN8]->SetNoteTable(m_iNoteLookupTableN106);

	m_pChannels[CHANID_S5B_CH1]->SetNoteTable(m_iNoteLookupTableS5B);
	m_pChannels[CHANID_S5B_CH2]->SetNoteTable(m_iNoteLookupTableS5B);
	m_pChannels[CHANID_S5B_CH3]->SetNoteTable(m_iNoteLookupTableS5B);*/
}

void SoundGen::generateVibratoTable(int type)
{
	for (int i = 0; i < 16; i++)	// depth
	{
		for (int j = 0; j < 16; j++)	// phase
		{
			int value = 0;
			double angle = (double(j) / 16.0) * (3.1415 / 2.0);

			if (type == VIBRATO_NEW)
			{
				value = int(std::sin(angle) * NEW_VIBRATO_DEPTH[i] /*+ 0.5*/);
			}
			else
			{
				value = (int)((double(j * OLD_VIBRATO_DEPTH[i]) / 16.0) + 1);
			}

			m_iVibratoTable[i * 16 + j] = value;
		}
	}
}

void SoundGen::resetTempo()
{
	if (m_pDocument == NULL)
		return;

	unsigned int speed = m_pDocument->GetSongSpeed();
	unsigned int tempo = m_pDocument->GetSongTempo();

	m_trackerctlr->setTempo(tempo, speed);
}

void SoundGen::addCycles(int count)
{
	// Add APU cycles
	m_iConsumedCycles += count;
	m_apu->AddTime(count);
}

void SoundGen::createChannels()
{
	// Clear all channels
	for (int i = 0; i < CHANNELS; i++)
	{
		m_pChannels[i] = NULL;
		m_pTrackerChannels[i] = NULL;
	}

	// 2A03/2A07
	assignChannel(CHANID_SQUARE1, new CSquare1Chan(this));
	assignChannel(CHANID_SQUARE2, new CSquare2Chan(this));
	assignChannel(CHANID_TRIANGLE, new CTriangleChan(this));
	assignChannel(CHANID_NOISE, new CNoiseChan(this));
	assignChannel(CHANID_DPCM, new CDPCMChan(this, m_samplemem));

	// Konami VRC6
	assignChannel(CHANID_VRC6_PULSE1, new CVRC6Square1(this));
	assignChannel(CHANID_VRC6_PULSE2, new CVRC6Square2(this));
	assignChannel(CHANID_VRC6_SAWTOOTH, new CVRC6Sawtooth(this));

	// Nintendo MMC5
	assignChannel(CHANID_MMC5_SQUARE1, new CMMC5Square1Chan(this));
	assignChannel(CHANID_MMC5_SQUARE2, new CMMC5Square2Chan(this));

	// Nintendo FDS
	assignChannel(CHANID_FDS, new CChannelHandlerFDS(this));

	// Konami VRC7
	assignChannel(CHANID_VRC7_CH1, new CVRC7Channel(this));
	assignChannel(CHANID_VRC7_CH2, new CVRC7Channel(this));
	assignChannel(CHANID_VRC7_CH3, new CVRC7Channel(this));
	assignChannel(CHANID_VRC7_CH4, new CVRC7Channel(this));
	assignChannel(CHANID_VRC7_CH5, new CVRC7Channel(this));
	assignChannel(CHANID_VRC7_CH6, new CVRC7Channel(this));

	// TODO - dan
/*
	// Namco N106
	assignChannel(new CTrackerChannel("Namco 1", SNDCHIP_N106, CHANID_N106_CHAN1), new CChannelHandlerN106(this));
	assignChannel(new CTrackerChannel("Namco 2", SNDCHIP_N106, CHANID_N106_CHAN2), new CChannelHandlerN106(this));
	assignChannel(new CTrackerChannel("Namco 3", SNDCHIP_N106, CHANID_N106_CHAN3), new CChannelHandlerN106(this));
	assignChannel(new CTrackerChannel("Namco 4", SNDCHIP_N106, CHANID_N106_CHAN4), new CChannelHandlerN106(this));
	assignChannel(new CTrackerChannel("Namco 5", SNDCHIP_N106, CHANID_N106_CHAN5), new CChannelHandlerN106(this));
	assignChannel(new CTrackerChannel("Namco 6", SNDCHIP_N106, CHANID_N106_CHAN6), new CChannelHandlerN106(this));
	assignChannel(new CTrackerChannel("Namco 7", SNDCHIP_N106, CHANID_N106_CHAN7), new CChannelHandlerN106(this));
	assignChannel(new CTrackerChannel("Namco 8", SNDCHIP_N106, CHANID_N106_CHAN8), new CChannelHandlerN106(this));

	// Sunsoft 5B
	assignChannel(new CTrackerChannel("Square 1", SNDCHIP_S5B, CHANID_S5B_CH1), new CS5BChannel1(this));
	assignChannel(new CTrackerChannel("Square 2", SNDCHIP_S5B, CHANID_S5B_CH2), new CS5BChannel2(this));
	assignChannel(new CTrackerChannel("Square 3", SNDCHIP_S5B, CHANID_S5B_CH3), new CS5BChannel3(this));
*/
}

void SoundGen::setupChannels()
{
	// Initialize channels
	for (int i = 0; i < CHANNELS; i++)
	{
		if (m_pChannels[i] != NULL)
		{
			m_pChannels[i]->InitChannel(m_apu, m_iVibratoTable, m_pDocument);
			m_pChannels[i]->SetVibratoStyle(m_pDocument->GetVibratoStyle());
			m_pChannels[i]->MakeSilent();
		}
	}
}

void SoundGen::assignChannel(int id, CChannelHandler *renderer)
{
	CTrackerChannel *trackerChannel = new CTrackerChannel;

	renderer->SetChannelID(id);

	m_pTrackerChannels[id] = trackerChannel;
	m_pChannels[id] = renderer;
}

void SoundGen::resetAPU()
{
	// Reset the APU
	m_apu->Reset();

	// Enable all channels
	m_apu->Write(0x4015, 0x0F);
	m_apu->Write(0x4017, 0x00);

	// MMC5
	m_apu->ExternalWrite(0x5015, 0x03);

	m_samplemem->SetMem(NULL, 0);
}

void SoundGen::playNote(int channel, stChanNote *noteData, int effColumns)
{
	if (noteData == NULL)
		return;

	// Update the individual channel
	m_pChannels[channel]->PlayNote(noteData, effColumns);
}

void SoundGen::runFrame()
{
	if (m_trackerActive)
	{
		m_trackerctlr->tick();
	}
}

void SoundGen::playSample(CDSample *sample, int offset, int pitch)
{
	m_samplemem->SetMem(sample->SampleData, sample->SampleSize);

	int loop=0;
	// (samplesize-1)/16 - offset*4
	int length = ((sample->SampleSize-1) >> 4) - (offset << 2);

	m_apu->Write(0x4010, pitch | loop);
	m_apu->Write(0x4012, offset);			// load address, start at $C000
	m_apu->Write(0x4013, length);			// length
	m_apu->Write(0x4015, 0x0F);
	m_apu->Write(0x4015, 0x1F);				// fire sample
}

void SoundGen::haltSounds()
{
	for (int i = 0; i < CHANNELS; i++)
	{
		if (m_pChannels[i] != NULL)
		{
			m_pChannels[i]->MakeSilent();
			m_pTrackerChannels[i]->Reset();
		}
	}
}

void SoundGen::requestFrame()
{
	runFrame();

	if (m_trackerActive)
	{
		m_bPlayerHalted = m_trackerctlr->isHalted();
	}

	for (int i = 0; i < CHANNELS; i++)
	{
		if (m_pChannels[i] == NULL)
			continue;

		if (m_pTrackerChannels[i]->NewNoteData())
		{
			stChanNote note = m_pTrackerChannels[i]->GetNote();

			playNote(i, &note, m_pDocument->GetEffColumns(i) + 1);
		}

		// Pitch wheel
		int pitch = m_pTrackerChannels[i]->GetPitch();
		m_pChannels[i]->SetPitch(pitch);

		// Update volume meters
		m_pTrackerChannels[i]->SetVolumeMeter(m_apu->GetVol(i));
	}

	const int CHANNEL_DELAY = 250;

	m_iConsumedCycles = 0;

	int frameRate = m_pDocument->GetFrameRate();

	// Update channels and channel registers
	for (int i = 0; i < CHANNELS; i++)
	{
		if (m_pChannels[i] != NULL)
		{
			m_pChannels[i]->ProcessChannel();
			m_pChannels[i]->RefreshChannel();
			m_apu->Process();
			// Add some delay between each channel update
			if (frameRate == CAPU::FRAME_RATE_NTSC || frameRate == CAPU::FRAME_RATE_PAL)
				addCycles(CHANNEL_DELAY);
		}
	}

	// Finish the audio frame
	m_apu->AddTime(m_iUpdateCycles - m_iConsumedCycles);
	m_apu->Process();
}

void SoundGen::apuCallback(const int16 *buf, uint32 sz, void *data)
{
	SoundGen *sg = (SoundGen*)data;
	sg->m_queued_sound->write(buf, sz);
}

core::u32 SoundGen::soundCallback(core::s16 *buf, core::u32 sz, void *data, core::u32 *idx)
{
	SoundGen *sg = (SoundGen*)data;
	return sg->requestSound(buf, sz, idx);
}

core::u32 SoundGen::requestSound(core::s16 *buf, core::u32 sz, core::u32 *idx)
{
	const core::u32 original_sz = sz;
	core::u32 c = 0;
	core::u32 off = 0;
	// read remaining sound buffer data from the last callback
	if (!m_queued_sound->isEmpty())
	{
		core::Quantity read = m_queued_sound->read(buf, sz);
		buf += read;
		sz -= read;
		off += read;
	}

	m_threading->mtx_running.lock();
	m_pDocument->lock();
	while (sz != 0)
	{
	/*	if (!m_bRunning)
		{
			// silence the rest of the buffer
			memset(buf, 0, sz*sizeof(core::s16));
		}*/
		requestFrame();
		bool haltsignal = m_bPlayerHalted && m_trackerActive;
		{
			idx[c++] = off;
			rowframe_t rf;
			rf.row = trackerController()->row();
			rf.frame = trackerController()->frame();
			rf.rowframe_changed = false;	// to be set when read from ringbuffer later
			rf.tracker_running = m_trackerActive;
			rf.halt_signal = haltsignal;

			core::u8 vols[MAX_CHANNELS];

			for (unsigned int i = 0; i < m_channels; i++)
			{
				int v = m_pActiveTrackerChannels[i]->GetVolumeMeter();
				vols[i] = v;
			}
			rf.volumes = writeVolume(vols);

			m_threading->mtx_rowframes.lock();
			m_queued_rowframes->write(&rf, 1);
			m_threading->mtx_rowframes.unlock();
		}

		if (haltsignal)
		{
			stopPlayback();
		}

		core::Quantity read = m_queued_sound->read(buf, sz);
		buf += read;
		sz -= read;
		off += read;
	}
	m_pDocument->unlock();

	if (m_sinkStopSamples > 0)
	{
		// stopping sink
		bool stop_playing = m_sinkStopSamples <= original_sz;
		m_sinkStopSamples = stop_playing ? 0 : (m_sinkStopSamples - original_sz);
		m_threading->mtx_running.unlock();

		if (stop_playing)
		{
			m_sink->setPlaying(false);
		}
	}
	else
	{
		m_threading->mtx_running.unlock();
	}

	return c;
}

const core::u8 *SoundGen::readVolume()
{
	const core::u8 *ptr = m_volumes_ring + m_volumes_read_offset * m_channels;

	m_volumes_read_offset++;
	m_volumes_read_offset %= m_volumes_size;

	return ptr;
}
const core::u8 * SoundGen::writeVolume(const core::u8 *arr)
{
	unsigned int sz = sizeof(core::u8)*m_channels;
	core::u8 *ptr = m_volumes_ring + m_volumes_write_offset * m_channels;
	memcpy(ptr, arr, sz);

	m_volumes_write_offset++;
	m_volumes_write_offset %= m_volumes_size;

	return ptr;
}

void SoundGen::timeCallback(core::u32 skip, void *data)
{
	SoundGen *sg = (SoundGen*)data;

	sg->m_threading->mtx_rowframes.lock();
	if (skip > 1)
	{
		sg->m_queued_rowframes->skipRead(skip-1);
	}
	rowframe_t rf;
	if (sg->m_queued_rowframes->read(&rf, 1) != 1)
	{
		sg->m_threading->mtx_rowframes.unlock();
		fprintf(stderr, "SoundGen::timeCallback(): ringbuffer underrun\n");
		// uh oh
		return;
	}
	sg->m_threading->mtx_rowframes.unlock();

	unsigned int row = rf.row;
	unsigned int frame = rf.frame;

	rf.rowframe_changed = (sg->m_lastRow != row) || (sg->m_lastFrame != frame);

	sg->m_lastRow = row;
	sg->m_lastFrame = frame;

	if (sg->m_trackerUpdateCallback != NULL)
		(*sg->m_trackerUpdateCallback)(rf, sg->trackerController()->document(), sg->m_trackerUpdateData);

	boost::unique_lock<boost::mutex> lock(sg->m_threading->mtx_tracker);
	if (sg->m_timer_trackerActive != rf.tracker_running)
	{
		sg->m_timer_trackerActive = rf.tracker_running;
		sg->m_threading->cond_trackerhalt.notify_all();
	}
}

void SoundGen::startPlayback()
{
	m_sinkStopSamples = -1;

	m_lastRow = ~0;
	m_lastFrame = ~0;

	m_bPlayerHalted = false;
	m_iFrameCounter = 0;

	m_pDocument->lock();
	m_channels = m_pDocument->GetAvailableChannels();

	m_volumes_size = rowframes_size;
	m_volumes_read_offset = 0;
	m_volumes_write_offset = 0;
	if (m_volumes_ring != NULL)
		delete[] m_volumes_ring;
	m_volumes_ring = new core::u8[m_volumes_size * m_channels];

	m_pDocument->unlock();

	setupChannels();
	resetTempo();

	m_sink->blockUntilTimerEmpty();
}
void SoundGen::stopPlayback()
{
	haltSounds();
	m_trackerActive = false;
	m_sinkStopSamples = m_sink->sampleRate() * 1/2;
}

void SoundGen::startTracker()
{
	m_threading->mtx_running.lock();

	if (!m_trackerActive)
	{
		m_trackerActive = true;

		startPlayback();

		m_threading->mtx_running.unlock();

		m_threading->mtx_sink.lock();
		m_sink->setPlaying(true);
		m_threading->mtx_sink.unlock();
	}
	else
	{
		m_threading->mtx_running.unlock();
	}
}
void SoundGen::stopTracker()
{
	boost::lock_guard<boost::mutex> lock(m_threading->mtx_running);

	if (m_trackerActive)
	{
		stopPlayback();
	}
}
bool SoundGen::isTrackerActive()
{
	m_threading->mtx_running.lock();
	bool active = m_trackerActive;
	m_threading->mtx_running.unlock();

	return active;
}
void SoundGen::blockUntilTrackerStopped()
{
	{
		boost::unique_lock<boost::mutex> lock(m_threading->mtx_tracker);
		while (m_timer_trackerActive)
		{
			// wait until tracker is inactive
			m_threading->cond_trackerhalt.wait(lock);
		}
	}
}

void SoundGen::auditionNote(int note, int octave, int instrument, int channel)
{
	// note = 0..11

	bool play = false;

	m_threading->mtx_running.lock();

	if (!m_trackerActive)
	{		
		startPlayback();

		stChanNote n;
		n.Note = note + C;
		n.Octave = octave;
		n.Instrument = instrument;
		n.Vol = 15;
		for (int i = 0; i < MAX_EFFECT_COLUMNS; i++)
		{
			n.EffNumber[i] = EF_NONE;
		}

		m_pActiveTrackerChannels[channel]->SetNote(n);
		play = true;
	}

	m_threading->mtx_running.unlock();

	if (play)
	{
		m_threading->mtx_sink.lock();
		m_sink->setPlaying(true);
		m_threading->mtx_sink.unlock();
	}
}
void SoundGen::auditionRow(unsigned int frame, unsigned int row)
{
	bool play = false;

	m_threading->mtx_running.lock();

	if (!m_trackerActive)
	{
		startPlayback();

		m_pDocument->lock();
		trackerController()->startAt(frame, row);
		trackerController()->playRow();
		m_pDocument->unlock();

		play = true;
	}

	m_threading->mtx_running.unlock();

	if (play)
	{
		m_threading->mtx_sink.lock();
		m_sink->setPlaying(true);
		m_threading->mtx_sink.unlock();
	}
}

void SoundGen::auditionHalt()
{
	m_threading->mtx_running.lock();
	if (!m_trackerActive)
	{
		stopPlayback();
	}
	m_threading->mtx_running.unlock();
}
