#include "exceptions.hpp"
#include <stdio.h>
#include <stdarg.h>

#ifdef _MSC_VER
#	define C_vsnprintf(a,b,c,d) vsnprintf_s(a,_vscprintf( c, d ),b,c,d)
#	define C_vsprintf(a,b,c) vsprintf_s(a,_vscprintf( b, c ), b,c)
#else
#	define C_vsnprintf(a,b,c,d) vsnprintf(a,b,c,d)
#	define C_vsprintf(a,b,c) vsprintf(a,b,c)
#endif

static char * init_helper(const char *fmt, va_list l)
{
	char *msg;
	va_list lc;

#ifdef _MSC_VER
	lc = l;
#else
	va_copy(lc, l);
#endif

	int len = C_vsnprintf(0, 0, fmt, l);

	l = lc;

	msg = new char[len+1];
	C_vsprintf(msg, fmt, l);
	va_end(l);

	return msg;
}

Exception::Exception(const char *fmt, ...) throw()
{
	va_list l;
	va_start(l, fmt);

	msg = init_helper(fmt, l);
}

Exception::~Exception() throw()
{
	if (msg != NULL)
		delete[] msg;
}

Fatal::Fatal(const char *fmt, ...) throw()
{
	va_list l;
	va_start(l, fmt);

	msg = init_helper(fmt, l);
}
