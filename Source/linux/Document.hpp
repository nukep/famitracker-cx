#ifndef _DOCUMENT_HPP_
#define _DOCUMENT_HPP_

#include <string>
#include "types.hpp"

class IO
{
public:
	virtual Quantity read(void *buf, Quantity sz) = 0;
	virtual Quantity write(const void *buf, Quantity sz) = 0;
	virtual ~IO(){ }

	bool read_e(void *buf, Quantity sz)
	{
		return read(buf, sz) == sz;
	}
	bool write_e(const void *buf, Quantity sz)
	{
		return write(buf, sz) == sz;
	}
	void read_exc(void *buf, Quantity sz)
	{
		ftm_Assert(read_e(buf, sz));
	}
	void write_exc(const void *buf, Quantity sz)
	{
		ftm_Assert(write_e(buf, sz));
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
};

class Document
{
public:
	Document();
	~Document();

	bool checkValidity(IO *io);

	unsigned int getFileVersion() const{ return m_iFileVersion; }

	unsigned int getBlockVersion() const{ return m_iBlockVersion; }
	int getBlockInt();
	char getBlockChar();
	bool blockDone() const{ return (m_iBlockPointer >= m_iBlockSize); }

	bool isFileDone() const{ return m_bFileDone; }

	void writeBlockInt(int);
	void writeBlockChar(char);

	const char *blockID() const{ return m_cBlockID; }
	bool readBlock(IO *io);
	void getBlock(void *buf, unsigned int size);
	void writeBlock(const void *data, unsigned int size);
	void createBlock(const char *id, int version);

	std::string readString();
private:
	unsigned int m_iFileVersion;

	char m_cBlockID[16];
	unsigned int m_iBlockSize;
	unsigned int m_iBlockVersion;
	char *m_pBlockData;

	unsigned int m_iMaxBlockSize;

	unsigned int m_iBlockPointer;
	bool m_bFileDone;

	void init_pBlockData(Quantity size);
	void reallocateBlock();
};

#endif

