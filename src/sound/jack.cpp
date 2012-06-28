#include <jack/jack.h>
#include <cmath>
#include <malloc.h>
#include <memory.h>
#include <stdio.h>
#include "jack.hpp"

core_api_SoundSink * sound_create()
{
	return new JackSound;
}

typedef jack_default_audio_sample_t sample_t;

struct jacksound_info_t
{
	JackSound * sink;
	jack_client_t * client;

	jack_port_t * out;

	core::s16 *buf;
};

static int process(jack_nframes_t frames, void *arg)
{
	jacksound_info_t *handle = (jacksound_info_t*)arg;

	jack_latency_range_t range;
	jack_port_get_latency_range(handle->out, JackPlaybackLatency, &range);
	core::u64 latency_us = range.max - frames*2;
	latency_us = latency_us * 1000000 / handle->sink->sampleRate();

	handle->sink->applyTime(latency_us);

	if (!handle->sink->isPlaying())
	{
		// don't play anything
		sample_t *buf = (sample_t*)jack_port_get_buffer(handle->out, frames);
		memset(buf, 0, frames*sizeof(sample_t));
		return 0;
	}

	handle->sink->performSoundCallback(handle->buf, frames);

	sample_t *buf = (sample_t*)jack_port_get_buffer(handle->out, frames);

	// convert s16 to float
	for (int i = 0; i < frames; i++)
	{
		buf[i] = sample_t(handle->buf[i])/sample_t(32768.0);
	}

	return 0;
}

JackSound::JackSound()
{
	m_handle = new jacksound_info_t;
	m_handle->sink = this;
	m_handle->client = NULL;
}
JackSound::~JackSound()
{
	JackSound::close();
	delete[] m_handle->buf;
	delete m_handle;
}
void JackSound::setPlaying(bool playing)
{
	SoundSink::setPlaying(playing);
}

void JackSound::initialize(unsigned int sampleRate, unsigned int channels, unsigned int latency_ms)
{
	if (m_handle->client != NULL)
	{
		JackSound::close();
	}

	m_handle->client = jack_client_open("FamiTracker", JackNullOption, NULL);
	if (!m_handle->client)
	{
		fprintf (stderr, "jack server not running?\n");
		return;
	}

	sampleRate = jack_get_sample_rate(m_handle->client);

	jack_set_process_callback(m_handle->client, process, m_handle);

	m_handle->out = jack_port_register(m_handle->client, "output", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);

	jack_nframes_t bufsize = jack_get_buffer_size(m_handle->client);
	m_handle->buf = new core::s16[bufsize];

	if (jack_activate(m_handle->client))
	{
		fprintf(stderr, "cannot activate client");
		return;
	}

	const char **ports;

	ports = jack_get_ports(m_handle->client, NULL, NULL, JackPortIsPhysical|JackPortIsInput);

	if (ports == NULL)
	{
		fprintf(stderr, "Cannot find any physical playback ports\n");
		return;
	}

	if (jack_connect(m_handle->client, jack_port_name(m_handle->out), ports[0]) != 0)
	{
		fprintf(stderr, "Cannot connect output ports\n");
	}
	if (jack_connect(m_handle->client, jack_port_name(m_handle->out), ports[1]) != 0)
	{
		fprintf(stderr, "Cannot connect output ports\n");
	}

	free(ports);
}
void JackSound::close()
{
	jack_client_close(m_handle->client);
	m_handle->client = NULL;
}
int JackSound::sampleRate() const
{
	return jack_get_sample_rate(m_handle->client);
}

// useless
void JackSound::flushBuffer(const core::s16 *Buffer, core::u32 Size)
{
}
void JackSound::flush()
{
}
