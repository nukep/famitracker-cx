#ifndef _JACK_HPP_
#define _JACK_HPP_

#include "famitracker-core/sound.hpp"

extern "C" SoundSink * sound_create();

struct jacksound_info_t;

class JackSound : public SoundSinkPlayback
{
public:
	JackSound();
	~JackSound();
	void initialize(unsigned int sampleRate, unsigned int channels, unsigned int latency_ms);
	void close();
	void FlushBuffer(int16 *Buffer, uint32 Size);
	void flush();

	int sampleRate() const;
private:
	jacksound_info_t * m_handle;
};

#endif
