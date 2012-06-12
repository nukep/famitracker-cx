#include <alsa/asoundlib.h>
#include <stdio.h>
#include "alsa.hpp"

SoundSink * sound_create()
{
	return new AlsaSound;
}

AlsaSound::AlsaSound()
	: m_handle(NULL)
{

}
AlsaSound::~AlsaSound()
{
	AlsaSound::close();
}

void AlsaSound::initialize(unsigned int sampleRate, unsigned int channels, unsigned int latency_ms)
{
	AlsaSound::close();

	snd_pcm_t *handle;

	int err;

	if ((err = snd_pcm_open(&handle, "default", SND_PCM_STREAM_PLAYBACK, 0)) < 0)
	{
		// error
		fprintf(stderr, "Playback open error: %s\n", snd_strerror(err));
		return;
	}

	if ((err = snd_pcm_set_params(handle,
								  SND_PCM_FORMAT_S16,
								  SND_PCM_ACCESS_RW_INTERLEAVED,
								  channels,
								  sampleRate,
								  1,
								  latency_ms*1000)) < 0)
	{
		// error
		fprintf(stderr, "Playback open error: %s\n", snd_strerror(err));
		return;
	}

	m_handle = handle;
}

void AlsaSound::close()
{
	if (m_handle != NULL)
	{
		snd_pcm_close((snd_pcm_t*)m_handle);
	}
}

void AlsaSound::FlushBuffer(int16 *buffer, uint32 size)
{
	snd_pcm_t *handle = (snd_pcm_t*)m_handle;
	snd_pcm_sframes_t frames;

	frames = snd_pcm_writei(handle, buffer, size);
	if (frames < 0)
	{
		frames = snd_pcm_recover(handle, frames, 0);
	}
	if (frames < 0)
	{
		// error
		fprintf(stderr, "snd_pcm_writei failed: %s\n", snd_strerror(frames));
		return;
	}
	if (frames > 0 && frames < (long)size)
	{
		// error
		fprintf(stderr, "Short write (expected %li, wrote %li)\n", (long)size, frames);
		return;
	}
}

void AlsaSound::flush()
{
	snd_pcm_t *handle = (snd_pcm_t*)m_handle;

	snd_pcm_prepare(handle);
}
