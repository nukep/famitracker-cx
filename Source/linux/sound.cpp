#include <stdlib.h>
#include <stdio.h>
#include <cmath>
#include "sound.hpp"
#include "FtmDocument.hpp"
#include "FamiTrackerTypes.h"
#include "TrackerChannel.h"
#include "TrackerController.hpp"

#include "ChannelHandler.h"
#include "Channels2A03.h"
#include "ChannelsVRC6.h"

// The depth of each vibrato level
static const double NEW_VIBRATO_DEPTH[] = {
	1.0, 1.5, 2.5, 4.0, 5.0, 7.0, 10.0, 12.0, 14.0, 17.0, 22.0, 30.0, 44.0, 64.0, 96.0, 128.0
};

static const double OLD_VIBRATO_DEPTH[] = {
	1.0, 1.0, 2.0, 3.0, 4.0, 7.0, 8.0, 15.0, 16.0, 31.0, 32.0, 63.0, 64.0, 127.0, 128.0, 255.0
};

SoundGen::SoundGen()
	: m_apu(&m_samplemem), m_iConsumedCycles(0), m_pDocument(NULL),
	  m_trackerUpdateCallback(NULL)
{
	// Create all kinds of channels
	createChannels();

	m_trackerctlr = new TrackerController;
}

SoundGen::~SoundGen()
{

}

void SoundGen::setSoundSink(SoundSink *s)
{
	m_sink = s;
	m_apu.SetCallback(s);
}

void SoundGen::setDocument(FtmDocument *doc)
{
	m_pDocument = doc;

	generateVibratoTable(doc->GetVibratoStyle());

	// TODO - dan: load settings
	m_apu.SetupSound(48000, 1, doc->GetMachine());
	m_apu.SetupMixer(16, 12000, 24, 100);

	loadMachineSettings(doc->GetMachine(), 0);

	unsigned char chip = doc->GetExpansionChip();
	m_apu.SetExternalSound(chip);
	resetAPU();

	resetTempo();

	m_lastRow = ~0;
	m_lastFrame = ~0;

	m_trackerctlr->initialize(doc, m_pTrackerChannels);

	m_iChannels = 0;
	for (int i = 0; i < CHANNELS; i++)
	{
		if (m_pTrackerChannels[i] && ((i < 5) || (m_pTrackerChannels[i]->GetChip() & chip)))
		{
			m_iChannels++;
		}
	}
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

	m_apu.ChangeMachine(machine == NTSC ? MACHINE_NTSC : MACHINE_PAL);

	// Choose a default rate if not predefined
	if (rate == 0)
		rate = DefaultRate;

	double clock_ntsc = CAPU::BASE_FREQ_NTSC / 16.0;
	double clock_pal = CAPU::BASE_FREQ_PAL / 16.0;

	for (int i = 0; i < NOTE_COUNT; ++i) {
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
/*	m_pChannels[CHANID_MMC5_SQUARE1]->SetNoteTable(m_iNoteLookupTableNTSC);
	m_pChannels[CHANID_MMC5_SQUARE2]->SetNoteTable(m_iNoteLookupTableNTSC);
	m_pChannels[CHANID_FDS]->SetNoteTable(m_iNoteLookupTableFDS);

	m_pChannels[CHANID_N106_CHAN1]->SetNoteTable(m_iNoteLookupTableN106);
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

void SoundGen::checkRenderStop()
{
	if (m_bRequestRenderStop)
		return;

	if (m_iRenderEndWhen == SONG_LOOP_LIMIT)
	{
		if (m_iRenderedFrames >= m_iRenderEndParam)
		{
			m_bRequestRenderStop = true;
		}
	}
}

void SoundGen::frameIsDone(int skipFrames)
{
	if (m_bRequestRenderStop)
		return;

	m_iRenderedFrames += skipFrames;
}

void SoundGen::addCycles(int count)
{
	// Add APU cycles
	m_iConsumedCycles += count;
	m_apu.AddTime(count);
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
	assignChannel(new CTrackerChannel("Square 1", SNDCHIP_NONE, CHANID_SQUARE1), new CSquare1Chan(this));
	assignChannel(new CTrackerChannel("Square 2", SNDCHIP_NONE, CHANID_SQUARE2), new CSquare2Chan(this));
	assignChannel(new CTrackerChannel("Triangle", SNDCHIP_NONE, CHANID_TRIANGLE), new CTriangleChan(this));
	assignChannel(new CTrackerChannel("Noise", SNDCHIP_NONE, CHANID_NOISE), new CNoiseChan(this));
	assignChannel(new CTrackerChannel("DPCM", SNDCHIP_NONE, CHANID_DPCM), new CDPCMChan(this, &m_samplemem));

	// Konami VRC6
	assignChannel(new CTrackerChannel("Square 1", SNDCHIP_VRC6, CHANID_VRC6_PULSE1), new CVRC6Square1(this));
	assignChannel(new CTrackerChannel("Square 2", SNDCHIP_VRC6, CHANID_VRC6_PULSE2), new CVRC6Square2(this));
	assignChannel(new CTrackerChannel("Sawtooth", SNDCHIP_VRC6, CHANID_VRC6_SAWTOOTH), new CVRC6Sawtooth(this));

	// TODO - dan
/*
	// Nintendo MMC5
	assignChannel(new CTrackerChannel("Square 1", SNDCHIP_MMC5, CHANID_MMC5_SQUARE1), new CMMC5Square1Chan(this));
	assignChannel(new CTrackerChannel("Square 2", SNDCHIP_MMC5, CHANID_MMC5_SQUARE2), new CMMC5Square2Chan(this));

	// Namco N106
	assignChannel(new CTrackerChannel("Namco 1", SNDCHIP_N106, CHANID_N106_CHAN1), new CChannelHandlerN106(this));
	assignChannel(new CTrackerChannel("Namco 2", SNDCHIP_N106, CHANID_N106_CHAN2), new CChannelHandlerN106(this));
	assignChannel(new CTrackerChannel("Namco 3", SNDCHIP_N106, CHANID_N106_CHAN3), new CChannelHandlerN106(this));
	assignChannel(new CTrackerChannel("Namco 4", SNDCHIP_N106, CHANID_N106_CHAN4), new CChannelHandlerN106(this));
	assignChannel(new CTrackerChannel("Namco 5", SNDCHIP_N106, CHANID_N106_CHAN5), new CChannelHandlerN106(this));
	assignChannel(new CTrackerChannel("Namco 6", SNDCHIP_N106, CHANID_N106_CHAN6), new CChannelHandlerN106(this));
	assignChannel(new CTrackerChannel("Namco 7", SNDCHIP_N106, CHANID_N106_CHAN7), new CChannelHandlerN106(this));
	assignChannel(new CTrackerChannel("Namco 8", SNDCHIP_N106, CHANID_N106_CHAN8), new CChannelHandlerN106(this));

	// Nintendo FDS
	assignChannel(new CTrackerChannel("FDS", SNDCHIP_FDS, CHANID_FDS), new CChannelHandlerFDS(this));

	// Konami VRC7
	assignChannel(new CTrackerChannel("FM Channel 1", SNDCHIP_VRC7, CHANID_VRC7_CH1), new CVRC7Channel(this));
	assignChannel(new CTrackerChannel("FM Channel 2", SNDCHIP_VRC7, CHANID_VRC7_CH2), new CVRC7Channel(this));
	assignChannel(new CTrackerChannel("FM Channel 3", SNDCHIP_VRC7, CHANID_VRC7_CH3), new CVRC7Channel(this));
	assignChannel(new CTrackerChannel("FM Channel 4", SNDCHIP_VRC7, CHANID_VRC7_CH4), new CVRC7Channel(this));
	assignChannel(new CTrackerChannel("FM Channel 5", SNDCHIP_VRC7, CHANID_VRC7_CH5), new CVRC7Channel(this));
	assignChannel(new CTrackerChannel("FM Channel 6", SNDCHIP_VRC7, CHANID_VRC7_CH6), new CVRC7Channel(this));

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
			m_pChannels[i]->InitChannel(&m_apu, m_iVibratoTable, m_pDocument);
			m_pChannels[i]->MakeSilent();
		}
	}
}

void SoundGen::assignChannel(CTrackerChannel *trackerChannel, CChannelHandler *renderer)
{
	int id = trackerChannel->GetID();

	renderer->SetChannelID(id);

	m_pTrackerChannels[id] = trackerChannel;
	m_pChannels[id] = renderer;
}

void SoundGen::resetAPU()
{
	// Reset the APU
	m_apu.Reset();

	// Enable all channels
	m_apu.Write(0x4015, 0x0F);
	m_apu.Write(0x4017, 0x00);

	// MMC5
	m_apu.ExternalWrite(0x5015, 0x03);

	m_samplemem.SetMem(NULL, 0);
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
	m_trackerctlr->tick();
}

void SoundGen::playSample(CDSample *sample, int offset, int pitch)
{
	m_samplemem.SetMem(sample->SampleData, sample->SampleSize);

	int loop=0;
	// (samplesize-1)/16 - offset*4
	int length = ((sample->SampleSize-1) >> 4) - (offset << 2);

	m_apu.Write(0x4010, pitch | loop);
	m_apu.Write(0x4012, offset);			// load address, start at $C000
	m_apu.Write(0x4013, length);			// length
	m_apu.Write(0x4015, 0x0F);
	m_apu.Write(0x4015, 0x1F);				// fire sample
}

void SoundGen::run()
{
	m_bRunning = true;
	m_bPlayerHalted = false;
	m_bPlaying = true;
	m_iDelayedStart = 0;
	m_iFrameCounter = 0;

	setupChannels();

	while (m_bRunning)
	{
		fflush(stdout);
		m_iFrameCounter++;

		runFrame();

		int channels = m_iChannels;

		for (int i = 0; i < channels; i++)
		{
			// TODO - dan, proper indexing
			int channel = m_pTrackerChannels[i]->GetID();

			if (m_pTrackerChannels[channel]->NewNoteData())
			{
				stChanNote note = m_pTrackerChannels[channel]->GetNote();

				playNote(channel, &note, m_pDocument->GetEffColumns(i) + 1);
			}

			// Pitch wheel
			int pitch = m_pTrackerChannels[channel]->GetPitch();
			m_pChannels[channel]->SetPitch(pitch);

			// Update volume meters
			m_pTrackerChannels[channel]->SetVolumeMeter(m_apu.GetVol(channel));
		}

		if (m_bPlayerHalted)
		{
			for (int i = 0; i < CHANNELS; i++)
			{
				if (m_pChannels[i] != NULL)
				{
					m_pChannels[i]->MakeSilent();
				}
			}
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
				m_apu.Process();
				// Add some delay between each channel update
				if (frameRate == CAPU::FRAME_RATE_NTSC || frameRate == CAPU::FRAME_RATE_PAL)
					addCycles(CHANNEL_DELAY);
			}
		}

		// Finish the audio frame
		m_apu.AddTime(m_iUpdateCycles - m_iConsumedCycles);
		m_apu.Process();

		unsigned int row = m_trackerctlr->row();
		unsigned int frame = m_trackerctlr->frame();

		bool updated = (m_lastRow != row) || (m_lastFrame != frame);

		if (updated && m_trackerUpdateCallback != NULL)
		{
			(*m_trackerUpdateCallback)(this);
		}

		m_lastRow = row;
		m_lastFrame = frame;

		// TODO - dan
/*		if (m_bPlayerHalted && m_bUpdateRow)
		{
			haltPlayer();
		}*/
	}
}
