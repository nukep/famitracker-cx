#ifndef DEFAULTS_HPP
#define DEFAULTS_HPP

#include "core/common/platform.hpp"

#if defined(LINUX)
#	define DEFAULT_SOUND "alsa"
#elif defined(WINDOWS)
#	define DEFAULT_SOUND "dx"
#endif

#if defined(UNIX)
#	if defined(MACOSX)
#		define SOUNDSINKLIB_FORMAT "libfamicx-core-%s-sound.dylib"
#	else
#		define SOUNDSINKLIB_FORMAT "libfamicx-core-%s-sound.so"
#	endif
#elif defined(WINDOWS)
#	define SOUNDSINKLIB_FORMAT "famicx-core-%s-sound.dll"
#else
#	error Unimplemented
#endif

#endif
