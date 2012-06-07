#ifndef _ALSA_HPP_
#define _ALSA_HPP_

#include "sound.hpp"

class AlsaSound : public SoundSinkPlayback
{
public:
	AlsaSound();
	~AlsaSound();
	void initialize(unsigned int sampleRate, unsigned int channels, unsigned int latency_ms);
	void close();
	void FlushBuffer(int16 *Buffer, uint32 Size);
private:
	void * m_handle;
};

#endif

