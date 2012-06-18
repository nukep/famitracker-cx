#ifndef _ALSA_HPP_
#define _ALSA_HPP_

#include "core/soundsink.hpp"

typedef core::SoundSink core_api_SoundSink;

extern "C" core_api_SoundSink * sound_create();

class AlsaSound : public core::SoundSinkPlayback
{
public:
	AlsaSound();
	~AlsaSound();
	void initialize(unsigned int sampleRate, unsigned int channels, unsigned int latency_ms);
	void close();
	void flushBuffer(core::s16 *Buffer, core::u32 Size);
	void flush();

	int sampleRate() const;
private:
	void * m_handle;
	int m_sampleRate;
};

#endif

