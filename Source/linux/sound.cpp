#include <stdlib.h>
#include "sound.hpp"

SoundGen::SoundGen()
	: m_apu(&m_samplemem)
{
}

SoundGen::~SoundGen()
{

}

void SoundGen::setSoundSink(SoundSink *s)
{
	m_sink = s;
	m_apu.SetCallback(s);
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
