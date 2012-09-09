#ifndef _DSOUND_HPP_
#define _DSOUND_HPP_

#include "core/soundsink.hpp"
#include "core/soundthread.hpp"

typedef core::SoundSink core_api_SoundSink;

extern "C" LIBEXPORT core_api_SoundSink * sound_create();

struct _dsound_threading;

class DSound : public core::SoundSinkPlayback
{
public:
	DSound();
	~DSound();
	void initialize(unsigned int sampleRate, unsigned int channels, unsigned int latency_ms);
	void close();
	void setPlaying(bool playing);

	int sampleRate() const;
private:
};

#endif
