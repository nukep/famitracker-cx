#include <boost/thread/mutex.hpp>
#include "dsound.hpp"
#include "core/time.hpp"

core_api_SoundSink * sound_create()
{
	return new DSound;
}

struct _dsound_threading
{
	boost::mutex mtx_running;
};

DSound::DSound()
	: m_directSound(NULL),
	  m_dsBuffer(NULL),
	  m_notifyEvent(NULL),
	  m_devices(0)
{
	m_threading = new _dsound_threading;
	m_notifyEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	m_dummyWindow = CreateWindow("STATIC", "FamiTracker", 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL);
}

DSound::~DSound()
{
	DestroyWindow(m_dummyWindow);
	CloseHandle(m_notifyEvent);
	m_thread.wait();
	DSound::close();
	setPlaying(false);

	delete m_threading;
}
void DSound::initialize(unsigned int sampleRate, unsigned int channels, unsigned int latency_ms)
{
	clearEnumeration();
	enumerate();

	if (FAILED(DirectSoundCreate((LPCGUID)m_GUIDs[0], &m_directSound, NULL)))
	{
		fprintf(stderr, "DSound: error initializing\n");
		return;
	}

	if (FAILED(m_directSound->SetCooperativeLevel(m_dummyWindow, DSSCL_PRIORITY)))
	{
		fprintf(stderr, "DSound: unable to set cooperation level\n");
		return;
	}

	m_latency_ms = latency_ms;
	m_bufferSize = sampleRate * latency_ms / 1000;

	openChannel(sampleRate);
}
void DSound::close()
{
	if (m_directSound == NULL)
		return;

	closeChannel();

	m_directSound->Release();
	m_directSound = NULL;

	clearEnumeration();
}
void DSound::callback(void *data)
{
	DSound *ds = (DSound*)data;
	ds->callbackO();
}
bool DSound::fillBuffer(DWORD offset, DWORD samples, DWORD *buf, unsigned int latency_us)
{
	DWORD *buf1, *buf2;
	DWORD rbs1, rbs2;
	HRESULT hRes;
	const DWORD samples_bytes = samples * sizeof(DWORD);
	hRes = m_dsBuffer->Lock(offset, samples_bytes, (LPVOID*)&buf1, &rbs1,
							(LPVOID*)&buf2, &rbs2, 0);

	if (FAILED(hRes))
		return false;

	printf("%u\n", samples);
	fflush(stdout);

	performSoundCallback((core::s16*)buf, samples);

	memcpy(buf1, buf, rbs1);
	if (buf2 != NULL)
	{
		memcpy(buf2, (char*)buf+rbs1, rbs2);
	}

	applyTime(latency_us);

	m_dsBuffer->Unlock(buf1, rbs1, buf2, rbs2);

	return true;
}

void DSound::callbackO()
{
	DWORD *buf = new DWORD[m_bufferSize];
	DWORD totalbufsz = m_bufferSize * sizeof(DWORD);
	m_dsBuffer->SetCurrentPosition(0);
	fillBuffer(0, m_bufferSize, buf, 0);
	m_dsBuffer->Play(NULL, NULL, DSBPLAY_LOOPING);

	unsigned int sr = sampleRate();

	while (true)
	{
		m_threading->mtx_running.lock();
		bool running = m_running;
		m_threading->mtx_running.unlock();
		if (!running)
		{
			break;
		}

		WaitForSingleObject(m_notifyEvent, INFINITE);

		// fill everything from currentWrite to currentPlay
		DWORD currentPlay, currentWrite;

		m_dsBuffer->GetCurrentPosition(&currentPlay, &currentWrite);

		DWORD bufsz;
		unsigned int samples;

		if (currentWrite > currentPlay)
		{
			bufsz = currentWrite - currentPlay;
		}
		else
		{
			bufsz = totalbufsz + currentWrite - currentPlay;
		}
		samples = (totalbufsz - bufsz)/sizeof(DWORD);
		unsigned int delay = (samples*1000 / sr) * 1000;

		if (!fillBuffer(currentWrite, bufsz/sizeof(DWORD), buf, delay))
			continue;
	}
	m_dsBuffer->Stop();
	m_dsBuffer->SetCurrentPosition(0);
	delete[] buf;
}

void DSound::setPlaying(bool playing)
{
	bool changed = playing != isPlaying();

	if (!changed)
		return;

	m_threading->mtx_running.lock();
	m_running = playing;
	m_threading->mtx_running.unlock();

	if (playing)
	{
		SoundSink::setPlaying(playing);
		m_thread.run(callback, this);
	}
	else
	{
		SoundSink::setPlaying(playing);
	}
}

int DSound::sampleRate() const
{
	return 48000;
	DWORD sr;
	m_dsBuffer->GetFrequency(&sr);
	return sr;
}

void DSound::enumerate()
{
	DirectSoundEnumerate(enumerateCallback, this);
}

BOOL CALLBACK DSound::enumerateCallback(LPGUID lpGuid, LPCSTR lpcstrDescription, LPCSTR lpcstrModule, LPVOID lpContext)
{
	DSound *o = (DSound*)lpContext;
	return o->enumerateCallbackO(lpGuid, lpcstrDescription, lpcstrModule);
}

BOOL DSound::enumerateCallbackO(LPGUID lpGuid, LPCSTR lpcstrDescription, LPCSTR lpcstrModule)
{
	m_cDevice[m_devices] = new char[strlen(lpcstrDescription) + 1];
	strcpy(m_cDevice[m_devices], lpcstrDescription);

	if (lpGuid != NULL)
	{
		m_GUIDs[m_devices] = new GUID;
		memcpy(m_GUIDs[m_devices], lpGuid, sizeof(GUID));
	}
	else
	{
		m_GUIDs[m_devices] = NULL;
	}

	m_devices++;
	return TRUE;
}

void DSound::clearEnumeration()
{
	for (unsigned int i = 0; i < m_devices; i++)
	{
		delete[] m_cDevice[i];
		if (m_GUIDs[i] != NULL)
			delete m_GUIDs[i];
	}

	m_devices = 0;
}

bool DSound::openChannel(unsigned int sampleRate)
{
	closeChannel();

	DSBPOSITIONNOTIFY dspn[2];
	WAVEFORMATEX wfx;
	DSBUFFERDESC dsbd;

	memset(&wfx, 0x00, sizeof(WAVEFORMATEX));
	wfx.cbSize				= sizeof(WAVEFORMATEX);
	wfx.nChannels			= 1;
	wfx.nSamplesPerSec		= sampleRate;
	wfx.wBitsPerSample		= 16;
	wfx.nBlockAlign			= wfx.nChannels * (wfx.wBitsPerSample / 8);
	wfx.nAvgBytesPerSec		= wfx.nSamplesPerSec * wfx.nBlockAlign;
	wfx.wFormatTag			= WAVE_FORMAT_PCM;

	memset(&dsbd, 0x00, sizeof(DSBUFFERDESC));
	dsbd.dwSize = sizeof(DSBUFFERDESC);
	dsbd.dwBufferBytes = m_bufferSize * sizeof(DWORD);
	dsbd.dwFlags = DSBCAPS_LOCSOFTWARE | DSBCAPS_GLOBALFOCUS | DSBCAPS_CTRLPOSITIONNOTIFY | DSBCAPS_GETCURRENTPOSITION2;
	dsbd.lpwfxFormat = &wfx;

	if (FAILED(m_directSound->CreateSoundBuffer(&dsbd, &m_dsBuffer, NULL)))
	{
		fprintf(stderr, "DSound: error opening channel\n");
		return false;
	}

	if (FAILED(m_dsBuffer->QueryInterface(IID_IDirectSoundNotify, (void**)&m_dsNotify)))
	{
		fprintf(stderr, "DSound: could not create notifier\n");
	}

	dspn[0].dwOffset = 0;
	dspn[0].hEventNotify = m_notifyEvent;
	dspn[1].dwOffset = (m_bufferSize / 2) * sizeof(DWORD);
	dspn[1].hEventNotify = m_notifyEvent;

	if (FAILED(m_dsNotify->SetNotificationPositions(2, dspn)))
	{
		fprintf(stderr, "DSound: could not set notification positions\n");
		return false;
	}

	return true;
}
void DSound::closeChannel()
{
	if (m_dsBuffer == NULL)
		return;

	m_dsBuffer->Release();
	m_dsNotify->Release();

	m_dsBuffer = NULL;
	m_dsNotify = NULL;
}
