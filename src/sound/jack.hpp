#ifndef _JACK_HPP_
#define _JACK_HPP_

#include "core/soundsink.hpp"

typedef core::SoundSink core_api_SoundSink;

extern "C" core_api_SoundSink * sound_create();

struct jacksound_info_t;

class JackSound : public core::SoundSinkPlayback
{
public:
	JackSound();
	~JackSound();
	void initialize(unsigned int sampleRate, unsigned int channels, unsigned int latency_ms);
	void close();
	void flushBuffer(core::s16 *Buffer, core::u32 Size);
	void flush();

	int sampleRate() const;
private:
	jacksound_info_t * m_handle;
};

#endif
