#include "dsound.hpp"

core_api_SoundSink * sound_create()
{
	return new DSound;
}

DSound::DSound()
{

}

DSound::~DSound()
{

}
void DSound::initialize(unsigned int sampleRate, unsigned int channels, unsigned int latency_ms)
{

}
void DSound::close()
{

}
void DSound::setPlaying(bool playing)
{
	SoundSinkPlayback::setPlaying(playing);
}

int DSound::sampleRate() const
{
	return 44100;
}
