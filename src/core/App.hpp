#ifndef _APP_HPP_
#define _APP_HPP_

#include "ChannelMap.h"
#include "Settings.h"

class SoundSink;

namespace app
{
	const CChannelMap * channelMap();
	const CSettings * settings();

	SoundSink * loadSoundSink(const char *name);
}

#endif

