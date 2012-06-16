#ifndef _ALSA_HPP_
#define _ALSA_HPP_

#include "famitracker-core/sound.hpp"

extern "C" SoundSink * sound_create();

class AlsaSound : public SoundSinkPlayback
{
public:
	AlsaSound();
	~AlsaSound();
	void initialize(unsigned int sampleRate, unsigned int channels, unsigned int latency_ms);
	void close();
	void FlushBuffer(int16 *Buffer, uint32 Size);
	void flush();

	int sampleRate() const;
private:
	void * m_handle;
	int m_sampleRate;
};

#endif

