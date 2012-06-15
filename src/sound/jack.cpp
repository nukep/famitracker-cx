#include <jack/jack.h>
#include <jack/ringbuffer.h>
#include <boost/thread/condition.hpp>
#include <boost/thread/mutex.hpp>
#include <cmath>
#include <malloc.h>
#include <stdio.h>
#include "jack.hpp"

typedef boost::mutex::scoped_lock lock;
boost::mutex monitor;
boost::condition cnd;

SoundSink * sound_create()
{
	return new JackSound;
}

typedef jack_default_audio_sample_t sample_t;

struct jacksound_info_t
{
	jack_client_t * client;

	jack_port_t * out;

	jack_ringbuffer_t *buffer;
};

static int process(jack_nframes_t frames, void *arg)
{
	jacksound_info_t *handle = (jacksound_info_t*)arg;

	void *buf = jack_port_get_buffer(handle->out, frames);

	int availableRead = jack_ringbuffer_read_space(handle->buffer);

	if (availableRead == 0)
		return 0;

	if (availableRead < frames*sizeof(sample_t))
		return 0;	// blip

	jack_ringbuffer_read(handle->buffer, (char*)buf, frames*sizeof(sample_t));

	cnd.notify_one();

	return 0;
}

JackSound::JackSound()
{
	m_handle = new jacksound_info_t;
	m_handle->client = NULL;
}
JackSound::~JackSound()
{
	JackSound::close();
	delete m_handle;
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
	m_handle->buffer = jack_ringbuffer_create(sampleRate*sizeof(sample_t));

	jack_ringbuffer_mlock(m_handle->buffer);

	jack_set_process_callback(m_handle->client, process, m_handle);

	m_handle->out = jack_port_register(m_handle->client, "output", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);

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
void JackSound::FlushBuffer(int16 *Buffer, uint32 Size)
{
//	int availableWrite = ;

	while (jack_ringbuffer_write_space(m_handle->buffer)/sizeof(sample_t) < Size)
	{
		// block until there's more room
		lock lk(monitor);
		cnd.wait(lk);
	}

	sample_t *src = new sample_t[Size];
	// convert s16 to float
	for (int i = 0; i < Size; i++)
	{
		src[i] = sample_t(Buffer[i])/sample_t(65535.0);
	}

	jack_ringbuffer_write(m_handle->buffer, (const char*)src, Size*sizeof(sample_t));
	delete[] src;
}
void JackSound::flush()
{
	jack_ringbuffer_reset(m_handle->buffer);
}
int JackSound::sampleRate() const
{
	return jack_get_sample_rate(m_handle->client);
}
