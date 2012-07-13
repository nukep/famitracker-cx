#include <stdio.h>
#include "io.hpp"

namespace core
{
	FileIO::FileIO(const char *filename, int flags)
	{
		const char *mode;

		if ( (flags & IO_READ) && !(flags & IO_WRITE) )
			mode = "rb";
		else if ( (flags & IO_READ) && (flags & IO_WRITE) )
			mode = "r+b";
		else if ( !(flags & IO_READ) && (flags & IO_WRITE) )
			mode = "wb";

		FILE *f = fopen(filename, mode);
		m_handle = f;
	}

	Quantity FileIO::read(void *buf, Quantity sz)
	{
		FILE *f = (FILE*)m_handle;
		if (f == NULL)
			return 0;
		return fread(buf, 1, sz, f);
	}

	Quantity FileIO::write(const void *buf, Quantity sz)
	{
		FILE *f = (FILE*)m_handle;
		if (f == NULL)
			return 0;
		return fwrite(buf, 1, sz, f);
	}

	Quantity FileIO::size()
	{
		FILE *f = (FILE*)m_handle;
		if (f == NULL)
			return 0;

		long lastpos = ftell(f);
		fseek(f, 0, SEEK_END);
		long sz = ftell(f);
		fseek(f, lastpos, SEEK_SET);

		return sz;
	}

	bool FileIO::seek(int offset, SeekOrigin origin)
	{
		FILE *f = (FILE*)m_handle;
		if (f == NULL)
			return false;

		int o;
		switch (origin)
		{
		case IO_SEEK_SET: o = SEEK_SET; break;
		case IO_SEEK_CUR: o = SEEK_CUR; break;
		case IO_SEEK_END: o = SEEK_END; break;
		}

		fseek(f, offset, o);
		return true;
	}
	bool FileIO::isReadable()
	{
		FILE *f = (FILE*)m_handle;
		if (f == NULL)
			return false;
		return true;
	}
	bool FileIO::isWritable()
	{
		FILE *f = (FILE*)m_handle;
		if (f == NULL)
			return false;
		return true;
	}

	FileIO::~FileIO()
	{
		FILE *f = (FILE*)m_handle;
		if (f != NULL)
		{
			fclose(f);
		}
	}
}
