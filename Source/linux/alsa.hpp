#ifndef _ALSA_HPP_
#define _ALSA_HPP_

#include "sound.hpp"

class AlsaSound : public SoundSink
{
public:
	void FlushBuffer(int16 *Buffer, uint32 Size);
};

#endif

