#ifndef _SOUND_HPP_
#define _SOUND_HPP_

#include "../APU/APU.H"

class SoundSink : public ICallback
{
};

class SoundGen
{
public:
	SoundGen();
	~SoundGen();

	void setSoundSink(SoundSink *s);

	void resetAPU();
private:
	CSampleMem m_samplemem;
	CAPU m_apu;
	SoundSink *m_sink;
};

#endif

