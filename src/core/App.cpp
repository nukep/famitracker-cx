#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <map>
#include <string>
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

	struct sound_handle_t
	{
		typedef SoundSink*(*create_f)();
		void *handle;
		create_f create;
	};

	typedef std::map<std::string, sound_handle_t> LoadedSoundSinksMap;
	LoadedSoundSinksMap loaded_sound_sinks;

	SoundSink * loadSoundSink(const char *name)
	{
		LoadedSoundSinksMap::const_iterator it = loaded_sound_sinks.find(name);
		if (it == loaded_sound_sinks.end())
		{
			// not found
			sound_handle_t h;
			char libname[128];
			sprintf(libname, "libfami-%s-sound.so", name);
			h.handle = dlopen(libname, RTLD_LAZY);
			if (h.handle == NULL)
			{
				fprintf(stderr, "loadSoundSink: Could not load: %s\n", dlerror());
				return NULL;
			}

			h.create = (sound_handle_t::create_f)dlsym(h.handle, "sound_create");
			if (h.create == NULL)
			{
				fprintf(stderr, "loadSoundSink: %s\n", dlerror());
				return NULL;
			}
			loaded_sound_sinks[name] = h;

			return (*h.create)();
		}
		else
		{
			const sound_handle_t &h = it->second;
			return (*h.create)();
		}
	}
}
