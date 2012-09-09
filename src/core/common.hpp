#include "common/platform.hpp"
#include "common/libraries.hpp"
#include "types.hpp"

#ifdef CORE_ISLIB
#	define COREAPI LIBEXPORT
#else
#	define COREAPI LIBIMPORT
#endif
