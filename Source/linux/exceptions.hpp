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

#endif

