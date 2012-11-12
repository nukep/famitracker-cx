#ifndef CORE_IO_HPP
#define CORE_IO_HPP

#include "common.hpp"

namespace core
{
	enum SeekOrigin
	{
		IO_SEEK_SET=0,
		IO_SEEK_CUR=1,
		IO_SEEK_END=2
	};

	enum
	{
		IO_READ=1,
		IO_WRITE=2,
		IO_READWRITE=3
	};

	class COREAPI IO
	{
	public:
		virtual Quantity read(void *buf, Quantity sz) = 0;
		virtual Quantity write(const void *buf, Quantity sz) = 0;
		virtual Quantity size() = 0;
		virtual bool seek(int offset, SeekOrigin o) = 0;
		virtual bool isReadable() = 0;
		virtual bool isWritable() = 0;
		virtual ~IO(){ }

		bool read_e(void *buf, Quantity sz)
		{
			return read(buf, sz) == sz;
		}
		bool write_e(const void *buf, Quantity sz)
		{
			return write(buf, sz) == sz;
		}

		bool readInt(int *i)
		{
			unsigned char buf[4];
			if (!read_e(buf, 4))
				return false;
			*i = (buf[3] << 24) | (buf[2] << 16) | (buf[1] << 8) | buf[0];
			return true;
		}
		bool readInt(unsigned int *i)
		{
			return readInt((int*)i);
		}
		bool writeInt(int i)
		{
			return write_e(&i, 4);
		}
		bool readChar(char &c)
		{
			return read_e(&c, 1);
		}
		bool readChar(signed char &c)
		{
			return read_e(&c, 1);
		}
		bool readChar(unsigned char &c)
		{
			return read_e(&c, 1);
		}
		bool writeChar(char c)
		{
			return write_e(&c, 1);
		}
		bool writeShort(short c)
		{
			return write_e(&c, 2);
		}
	};

	class LIBEXPORT FileIO : public IO
	{
	public:
		FileIO(const char *filename, int flags);
		Quantity read(void *buf, Quantity sz);
		Quantity write(const void *buf, Quantity sz);
		Quantity size();
		bool seek(int offset, SeekOrigin o);
		bool isReadable();
		bool isWritable();
		~FileIO();
	private:
		void *m_handle;
	};
}

#endif
