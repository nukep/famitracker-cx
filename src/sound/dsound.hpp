#ifndef _DSOUND_HPP_
#define _DSOUND_HPP_

#include "core/soundsink.hpp"
#include "core/soundthread.hpp"
#include <Windows.h>
#include <InitGuid.h>
#include <dsound.h>

typedef core::SoundSink core_api_SoundSink;

extern "C" LIBEXPORT core_api_SoundSink * sound_create();

struct _dsound_threading;

static const unsigned int MAX_DEVICES = 256;

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
	static void callback(void *);
	void callbackO();
	bool fillBuffer(DWORD offset, DWORD sz, DWORD *buf, unsigned int latency_us);
	void enumerate();
	void clearEnumeration();
	static BOOL CALLBACK enumerateCallback(LPGUID lpGuid, LPCSTR lpcstrDescription, LPCSTR lpcstrModule, LPVOID lpContext);
	BOOL enumerateCallbackO(LPGUID lpGuid, LPCSTR lpcstrDescription, LPCSTR lpcstrModule);

	bool openChannel(unsigned int sampleRate);
	void closeChannel();

	HWND m_dummyWindow;
	LPDIRECTSOUND m_directSound;
	LPDIRECTSOUNDBUFFER m_dsBuffer;
	LPDIRECTSOUNDNOTIFY m_dsNotify;
	HANDLE m_notifyEvent;

	unsigned int m_devices;
	char *m_cDevice[MAX_DEVICES];
	GUID *m_GUIDs[MAX_DEVICES];

	core::SoundThread m_thread;
	_dsound_threading *m_threading;
	bool m_running;

	unsigned int m_latency_ms;
	unsigned int m_bufferSize;	// in samples
};

#endif
