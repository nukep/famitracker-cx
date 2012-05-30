#include <stdlib.h>
#include <cmath>
#include "sound.hpp"
#include "FamiTrackerTypes.h"
#include "TrackerChannel.h"

#include "ChannelHandler.h"
#include "Channels2A03.h"
//#include "ChannelsVRC6.h"

// The depth of each vibrato level
static const double NEW_VIBRATO_DEPTH[] = {
	1.0, 1.5, 2.5, 4.0, 5.0, 7.0, 10.0, 12.0, 14.0, 17.0, 22.0, 30.0, 44.0, 64.0, 96.0, 128.0
};

static const double OLD_VIBRATO_DEPTH[] = {
	1.0, 1.0, 2.0, 3.0, 4.0, 7.0, 8.0, 15.0, 16.0, 31.0, 32.0, 63.0, 64.0, 127.0, 128.0, 255.0
};

SoundGen::SoundGen()
	: m_apu(&m_samplemem), m_iConsumedCycles(0)
{
	// Create all kinds of channels
	createChannels();
}

SoundGen::~SoundGen()
{

}

void SoundGen::setSoundSink(SoundSink *s)
{
	m_sink = s;
	m_apu.SetCallback(s);
}

void SoundGen::generateVibratoTable(int type)
{
	for (int i=0;i<16;i++)	// depth
	{
		for (int j=0;j<16;j++)	// phase
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

void SoundGen::evaluateGlobalEffects(stChanNote *noteData, int effColumns)
{
	// Handle global effects (effects that affects all channels)
	for (int i=0;i<effColumns;i++)
	{
		unsigned char effNum   = noteData->EffNumber[i];
		unsigned char effParam = noteData->EffParam[i];

		switch (effNum)
		{
			// Fxx: Sets speed to xx
			case EF_SPEED:
				if (!effParam)
					effParam++;
				if (effParam > MIN_TEMPO)
					m_iTempo = effParam;
				else
					m_iSpeed = effParam;
				m_iTempoDecrement = (m_iTempo * 24) / m_iSpeed;  // 24 = 6 * 4
				break;

			// Bxx: Jump to pattern xx
			case EF_JUMP:
				m_iJumpToPattern = effParam;
				frameIsDone(1);
				if (m_bRendering)
					checkRenderStop();
				frameIsDone(m_iJumpToPattern);
				if (m_bRendering)
					checkRenderStop();
				break;

			// Dxx: Skip to next track and start at row xx
			case EF_SKIP:
				m_iSkipToRow = effParam;
				frameIsDone(1);
				if (m_bRendering)
					checkRenderStop();
				break;

			// Cxx: Halt playback
			case EF_HALT:
				m_bPlayerHalted = true;
				//HaltPlayer();
				if (m_bRendering)
				{
					// Unconditional stop
					m_iRenderedFrames++;
					m_bRequestRenderStop = true;
				}
				break;
		}
	}
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
	for (int i=0;i<CHANNELS;i++)
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
/*
	// Konami VRC6
	assignChannel(new CTrackerChannel("Square 1", SNDCHIP_VRC6, CHANID_VRC6_PULSE1), new CVRC6Square1(this));
	assignChannel(new CTrackerChannel("Square 2", SNDCHIP_VRC6, CHANID_VRC6_PULSE2), new CVRC6Square2(this));
	assignChannel(new CTrackerChannel("Sawtooth", SNDCHIP_VRC6, CHANID_VRC6_SAWTOOTH), new CVRC6Sawtooth(this));
*/
	// TODO - dan
/*
	// Konami VRC7
	assignChannel(new CTrackerChannel("FM Channel 1", SNDCHIP_VRC7, CHANID_VRC7_CH1), new CVRC7Channel(this));
	assignChannel(new CTrackerChannel("FM Channel 2", SNDCHIP_VRC7, CHANID_VRC7_CH2), new CVRC7Channel(this));
	assignChannel(new CTrackerChannel("FM Channel 3", SNDCHIP_VRC7, CHANID_VRC7_CH3), new CVRC7Channel(this));
	assignChannel(new CTrackerChannel("FM Channel 4", SNDCHIP_VRC7, CHANID_VRC7_CH4), new CVRC7Channel(this));
	assignChannel(new CTrackerChannel("FM Channel 5", SNDCHIP_VRC7, CHANID_VRC7_CH5), new CVRC7Channel(this));
	assignChannel(new CTrackerChannel("FM Channel 6", SNDCHIP_VRC7, CHANID_VRC7_CH6), new CVRC7Channel(this));

	// Nintendo FDS
	assignChannel(new CTrackerChannel("FDS", SNDCHIP_FDS, CHANID_FDS), new CChannelHandlerFDS(this));

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

	// Sunsoft 5B
	assignChannel(new CTrackerChannel("Square 1", SNDCHIP_S5B, CHANID_S5B_CH1), new CS5BChannel1(this));
	assignChannel(new CTrackerChannel("Square 2", SNDCHIP_S5B, CHANID_S5B_CH2), new CS5BChannel2(this));
	assignChannel(new CTrackerChannel("Square 3", SNDCHIP_S5B, CHANID_S5B_CH3), new CS5BChannel3(this));
*/
}

void SoundGen::assignChannel(CTrackerChannel *trackerChannel, CChannelHandler *renderer)
{
	int id = trackerChannel->GetID();

	// TODO - dan
//	renderer->SetChannelID(id);

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
