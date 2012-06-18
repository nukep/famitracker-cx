#include <stdio.h>
#include <dlfcn.h>
#include <map>
#include <string>
#include "soundsink.hpp"

namespace core
{
	struct sound_handle_t
	{
		typedef core::SoundSink*(*create_f)();
		void *handle;
		create_f create;
	};

	typedef std::map<std::string, sound_handle_t> LoadedSoundSinksMap;
	static LoadedSoundSinksMap loaded_sound_sinks;

	core::SoundSink * loadSoundSink(const char *name)
	{
		LoadedSoundSinksMap::const_iterator it = loaded_sound_sinks.find(name);
		if (it == loaded_sound_sinks.end())
		{
			// not found
			sound_handle_t h;
			char libname[128];
			sprintf(libname, "libcore-%s-sound.so", name);
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
