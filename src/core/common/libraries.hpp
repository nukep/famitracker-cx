/*
	this file doesn't have include guards.  that's so the programmer can
	include this file at different times, asking for different libraries.
	eg.
	#define INC_LIBA
	#include "common.h"  // example header, which contains #include "common/libraries.hpp"
	...
	#define INC_LIBZ
	#include "common.h"

	try to include this file by proxy from another include file.
	at that stage, one can prevent unsupported libraries from being used
	eg.  #ifndef USE_LIBA   #ifdef INC_LIBA   #undef INC_LIBA   #endif   #endif
*/

// all INC_... defines must be defined before you include this file!

#include "platform.hpp"

#ifdef INC_MEM
#	include <memory.h>
//#	include <malloc.h>
// Mac OS X deprecates malloc.h (as in won't find it). stdlib.h should be used anyway.
#	include <stdlib.h>

#	if defined(X86X64)
#		include <emmintrin.h>
// _mm_malloc works in visual c++, gcc, mingw-gcc
#		define C_memalign(align, size) _mm_malloc(size, align)
#		define C_memalign_free(ptr) _mm_free(ptr)
#	else//if defined(ARM)
#		define C_memalign(align, size) memalign(align, size)
#		define C_memalign_free(ptr) free(ptr)
#	endif

#endif

#ifdef INC_MATH
	#ifndef CINC_MATH
		#define CINC_MATH

		#ifdef _MSC_VER
			#undef _USE_MATH_DEFINES
			#define _USE_MATH_DEFINES
		#endif

		#include <cmath>
		#include <algorithm>

		namespace math
		{
			#ifdef _MSC_VER
				static inline float round(float val)
				{
					return std::floor(val + 0.5f);
				}
				static inline double round(double val)
				{
					return std::floor(val + 0.5);
				}
			#else
				static inline float round(float val)
				{
					return ::roundf(val);
				}
				static inline double round(double val)
				{
					return ::round(val);
				}
			#endif

			template <typename T>
			static inline T _helper_mod(T q, T div)
			{
				return q % div;
			}
			static inline float _helper_mod(float q, float div)
			{
				return ::fmodf(q, div);
			}
			static inline double _helper_mod(double q, double div)
			{
				return ::fmod(q, div);
			}

			template <typename T>
			static T mod(T q, T div)
			{
				// specializing mod didn't work the intended way (eg. couldn't do float r = mod<float>(a, b);)
				return _helper_mod(q, div);
			}

			template <typename T>
			static T positive_mod(T q, T div)
			{
				// div has no effect on a negative result, only q does
				T r = mod(q, div);
				if (r < 0) r += div;
				ASSUME_NEVER(r < 0);	// can't happen
				ASSUME_NEVER(r >= div);
				return r;
			}

			// 2**n
			static int pow2(int n)
			{
				if (n < 0)
					return 0;
				return 1 << n;
			}
			static unsigned int pow2(unsigned int n)
			{
				return 1 << n;
			}
			static float pow2(float n)
			{
				return std::pow(2.0f, n);
			}
			static double pow2(double n)
			{
				return std::pow(2.0, n);
			}

			template <typename T>
			static T _helper_positive_mod_pow2(T q, T raise)
			{
				unsigned int a = pow2((unsigned int)raise)-1;
				return q & a;
			}
			static float _helper_positive_mod_pow2(float q, float raise)
			{
				return positive_mod(q, pow2(raise));
			}
			static double _helper_positive_mod_pow2(double q, double raise)
			{
				return positive_mod(q, pow2(raise));
			}
			template <typename T>
			static T _helper_mod_pow2(T q, T raise)
			{
				unsigned int p = pow2((unsigned int)raise);
				unsigned int a = p-1;
				if (q < 0)
				{
					T r = (-q) & a;
					return -r;
				}
				else
				{
					return q & a;
				}
			}
			static float _helper_mod_pow2(float q, float raise)
			{
				return mod(q, pow2(raise));
			}
			static double _helper_mod_pow2(double q, double raise)
			{
				return mod(q, pow2(raise));
			}

			template <typename T>
			static T positive_mod_pow2(T q, T raise)
			{
				return _helper_positive_mod_pow2(q, raise);
			}

			template <typename T>
			static T mod_pow2(T q, T raise)
			{
				return _helper_mod_pow2(q, raise);
			}

			template <typename T>
			static T fraction(T n)
			{
				T d;
				return std::modf(n, &d);
			}


			template <typename T>
			static inline T clamp(T val, T min, T max)
			{
				if (val <= min)
					return min;
				if (val >= max)
					return max;

				return val;
			}

		}
	#endif
#endif
