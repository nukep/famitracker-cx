#ifndef _COMMON_HPP_
#define _COMMON_HPP_

#include "../core/common/platform.hpp"
#include "Common.h"

#ifdef FAMICORE_ISLIB
#	define FAMICOREAPI LIBEXPORT
#else
#	define FAMICOREAPI LIBIMPORT
#endif

#endif

