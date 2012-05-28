#ifndef _TYPES_HPP_
#define _TYPES_HPP_

#include "exceptions.hpp"

typedef unsigned int Quantity;

// Releasing pointers
#define SAFE_RELEASE(p) \
	if (p != NULL) { \
		delete p;	\
		p = NULL;	\
	}	\

#define SAFE_RELEASE_ARRAY(p) \
	if (p != NULL) { \
		delete [] p;	\
		p = NULL;	\
	}	\

#if defined(__GNUC__)
#	define FUNCTION_NAME __PRETTY_FUNCTION__
#elif defined(_MSC_VER)
#	define FUNCTION_NAME __FUNCTION__
#endif

#define _ftm_Assert_msg(truth) throw Fatal("Assertion fail: %s on line %d, %s: \"%s\"", __FILE__, __LINE__, FUNCTION_NAME, #truth)
#define ftm_Assert(truth) if ((! (truth) )) _ftm_Assert_msg(truth)

#endif

