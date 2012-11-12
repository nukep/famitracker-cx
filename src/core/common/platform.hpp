#ifndef COMMON_PLATFORM_HPP
#define COMMON_PLATFORM_HPP
	/*
		note: anything clang-specific should be checked with __clang__ first,
		as clang seems to define __GNUC__.
		do this:  #if defined(__clang__) ... #elif defined(__GNUC__)
		and not:  #if defined(__GNUC__) ... #elif defined(__clang__)
	*/

	#ifndef NULL
	#	define NULL 0
	#endif

	#if defined _WIN32 || defined __CYGWIN__
	#	define WINDOWS
	#elif defined __APPLE__
	#	define MACOSX
	#elif defined __linux__
	#	define LINUX
	#else
	#	error Unsupported OS.  Please refer to platform.hpp
	#endif

	#ifdef __unix__
	#	define UNIX
	#endif
		
	#if defined(__i386__) || defined(_M_IX86)
	#	define X86
	#	define X86X64
	#endif

	#if defined(__amd64__) || defined(_M_X64)
	#	define X64
	#	define X86X64
	#endif
	
	#if defined(__arm__)
	#	define ARM
	#endif

	#ifdef X86X64
	#	if !(defined __LP64__ || defined __LLP64__) || defined _WIN32 && !defined _WIN64
	#		define _32BIT
	#	else
	#		define _64BIT
	#	endif
	#elif defined(ARM)
	#	define _32BIT
	#endif

    // NativeInt is as long as the architecture's general purpose register type
    // (as much as we can get without sacrificing performance)
    #if defined(_32BIT)
        typedef unsigned long NativeInt;
    #elif defined(_64BIT)
        typedef unsigned long long NativeInt;
    #endif
	
	#ifdef UNIX
	// we'll assume this is the case
	#	define POSIX
	#endif

	#ifdef WINDOWS
	#	define CASEINSENSITIVE_FILES
	#else
	#	define CASESENSITIVE_FILES
	#endif

	#define C_strcmp(a, b) strcmp(a, b)

#	ifdef _MSC_VER
	// compiling under Visual C++ (and hence windows)
#		define PRAGMA(x) __pragma(x)
#		define C_strcasecmp(a, b) _stricmp(a, b)
#		define C_strdup(a) _strdup(a)
#	else
	// other compilers (GCC, clang, etc.)
#		define PRAGMA(x) _Pragma(#x)
#		define C_strcasecmp(a, b) strcasecmp(a, b)
#		define C_strdup(a) strdup(a)
#	endif

#	ifdef _MSC_VER
#		define RESTRICT __restrict
#	elif defined(__GNUC__)
#		define RESTRICT __restrict__
		// GCC_VERSION gives us one number to compare to. eg. gcc 4.6.0 is 40600
#		define GCC_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
#	endif

#	if defined(__GNUC__)
#		define LIKELY(expr) __builtin_expect((expr), !0)
#		define UNLIKELY(expr) __builtin_expect((expr), 0)
#	else
#		define LIKELY(expr) (expr)
#		define UNLIKELY(expr) (expr)
#	endif

#	if defined(__GNUC__)
#		define LIBIMPORT
#		define LIBEXPORT
#	elif defined(_MSC_VER)
#		define LIBIMPORT __declspec(dllimport)
#		define LIBEXPORT __declspec(dllexport)
#	endif

#	if GCC_VERSION >= 40500
		// only gcc 4.5 and above supports this
#		define ASSUME(truth) if (!(truth)) __builtin_unreachable()
#		define ASSUME_NEVER(lie) if (lie) __builtin_unreachable()
#		define UNREACHABLE __builtin_unreachable()
#	elif defined (_MSC_VER)
#		define ASSUME(truth) __assume(truth)
#		define ASSUME_NEVER(lie) if (lie) __assume(0)
#		define UNREACHABLE __assume(0)
#	else
#		define ASSUME(truth)
#		define ASSUME_NEVER(lie)
#		define UNREACHABLE
#	endif

#endif	// COMMON_H

