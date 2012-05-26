#ifndef _TYPES_HPP_
#define _TYPES_HPP_

#include <assert.h>

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

#define ftm_Assert(truth) assert(truth)

#endif

