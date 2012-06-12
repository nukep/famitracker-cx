#ifndef _EXCEPTIONS_HPP_
#define _EXCEPTIONS_HPP_

#include <exception>

class Exception : public std::exception
{
public:
	Exception() throw() : msg(0)
	{
	}

	Exception(const char *fmt, ...) throw();
	const char *what() const throw()
	{
		return msg;
	}
	~Exception() throw();

protected:
	char *msg;
};

class Fatal : public Exception
{
public:
	Fatal(const char *fmt, ...) throw();
};

#if defined(__GNUC__)
#	define FUNCTION_NAME __PRETTY_FUNCTION__
#elif defined(_MSC_VER)
#	define FUNCTION_NAME __FUNCTION__
#endif

#define _ftm_Assert_msg(truth) throw Fatal("Assertion fail: %s on line %d, %s: \"%s\"", __FILE__, __LINE__, FUNCTION_NAME, #truth)
#define ftm_Assert(truth) if ((! (truth) )) _ftm_Assert_msg(truth)

#define ftkr_Assert(truth) ftm_Assert(truth)

#endif

