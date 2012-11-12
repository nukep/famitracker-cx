#ifndef _APP_HPP_
#define _APP_HPP_

#include "ChannelMap.h"
#include "Settings.h"
#include "common.hpp"

namespace core
{
	class SoundSink;
}

namespace app
{
	FAMICOREAPI const CChannelMap * channelMap();
	FAMICOREAPI const CSettings * settings();
}

#endif

