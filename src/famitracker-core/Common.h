/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2010  Jonathan Liss
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful, 
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
** Library General Public License for more details.  To obtain a 
** copy of the GNU Library General Public License, write to the Free 
** Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** Any permitted reproduction of these routines, in whole or in part,
** must bear this legend.
*/

#pragma once

#include <string.h>

#if defined _WIN32 || defined __CYGWIN__
#	define WINDOWS
#elif defined __linux__
#	define LINUX
#	include <stdint.h>
#endif

typedef unsigned char		uint8;
typedef unsigned short		uint16;
typedef unsigned long		uint32;
#ifdef WINDOWS
typedef unsigned __int64	uint64;
#else
typedef uint64_t uint64;
#endif
typedef signed char			int8;
typedef signed short		int16;
typedef signed long			int32;
#ifdef WINDOWS
typedef signed __int64		int64;
#else
typedef int64_t int64;
#endif

#define _MAIN_H_

#define SAMPLES_IN_BYTES(x) (x << SampleSizeShift)

const int SPEED_AUTO	= 0;
const int SPEED_NTSC	= 1;
const int SPEED_PAL		= 2;


// Used to play the audio when the buffer is full
class ICallback {
public:
};


// class for simulating CPU memory, used by the DPCM channel
class CSampleMem 
{
	public:
		uint8 Read(uint16 Address) {
			if (!m_pMemory)
				return 0;
			uint16 Addr = (Address - 0xC000);// % m_iMemSize;
			if (Addr >= m_iMemSize)
				return 0;
			return m_pMemory[Addr];
		}

		void SetMem(char *Ptr, int Size) {
			m_pMemory = (uint8*)Ptr;
			m_iMemSize = Size;
		}

	private:
		uint8  *m_pMemory;
		uint16	m_iMemSize;
};

// Safe string copy, always null terminates.
// dst_sz is the size of the entire dst buffer, including last null character
static inline void safe_strcpy(char *dst, const char *src, size_t dst_sz)
{
#ifdef WINDOWS
	strcpy_s(dst, dst_sz, src);
#else
	strncpy(dst, src, dst_sz-1);
	dst[dst_sz-1] = 0;
#endif
}
