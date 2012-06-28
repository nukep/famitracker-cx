#ifndef CORE_TIME_HPP
#define CORE_TIME_HPP

#include "common.hpp"

#include <stdio.h>

#if defined(UNIX)
#include <sys/time.h>

namespace core
{
	struct timestamp_t
	{
		void gettime()
		{
			gettimeofday(&tv, NULL);
		}
		int diff_us(const timestamp_t &before) const
		{
			int s = tv.tv_sec - before.tv.tv_sec;
			int u = tv.tv_usec - before.tv.tv_usec;

			return s*1000000 + u;
		}
		int diff_ms(const timestamp_t &before) const
		{
			int s = tv.tv_sec - before.tv.tv_sec;
			int m = (tv.tv_usec - before.tv.tv_usec)/1000;

			return s*1000 + m;
		}
		bool isLessThan(const timestamp_t &before) const
		{
			if (tv.tv_sec < before.tv.tv_sec)
				return true;
			if (tv.tv_sec > before.tv.tv_sec)
				return false;
			if (tv.tv_usec < before.tv.tv_usec)
				return true;

			return false;
		}
		timestamp_t add_us(unsigned int us) const
		{
			timestamp_t t = *this;
			t.tv.tv_usec += us;
			t.tv.tv_sec += t.tv.tv_usec/1000000;
			t.tv.tv_usec %= 1000000;
			return t;
		}
		timestamp_t add_ms(unsigned int ms) const
		{
			return add_us(ms*1000);
		}

		void debug_print(FILE *out) const
		{
			fprintf(out, "sec: %ld\tusec: %6ld\n", tv.tv_sec, tv.tv_usec);
		}
	private:
		struct timeval tv;
	};

	static inline void sleep_us(unsigned int us)
	{
		usleep(us);
	}
}
#elif defined(WINDOWS)
#error Unimplemented
#endif

#endif

