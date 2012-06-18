#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "App.hpp"

namespace app
{
	static CChannelMap channel_map;
	static CSettings app_settings;

	const CChannelMap * channelMap()
	{
		return &channel_map;
	}

	const CSettings * settings()
	{
		return &app_settings;
	}
}
