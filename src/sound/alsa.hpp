#ifndef _ALSA_HPP_
#define _ALSA_HPP_

#include <alsa/asoundlib.h>
#include "core/soundsink.hpp"
#include "soundthread.hpp"

typedef core::SoundSink core_api_SoundSink;

extern "C" core_api_SoundSink * sound_create();

struct _alsasound_threading;

class AlsaSound : public core::SoundSinkPlayback
{
public:
	AlsaSound();
	~AlsaSound();
	void initialize(unsigned int sampleRate, unsigned int channels, unsigned int latency_ms);
	void close();
	void setPlaying(bool playing);

	int sampleRate() const;
private:
	void callback();
	static void callback_bootstrap(void *);
	snd_pcm_t * m_handle;
	snd_pcm_uframes_t m_buffer_size, m_period_size;
	int m_sampleRate;
	SoundThread m_thread;
	_alsasound_threading * m_threading;
	bool m_running;
};

#endif

