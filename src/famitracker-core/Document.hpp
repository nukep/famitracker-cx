#ifndef _DOCUMENT_HPP_
#define _DOCUMENT_HPP_

#include <string>
#include "types.hpp"
#include "core/io.hpp"

class Document
{
public:
	Document();
	~Document();

	void setIO(core::IO *io){ m_io = io; }

	bool checkValidity();

	unsigned int getFileVersion() const{ return m_iFileVersion; }

	unsigned int getBlockVersion() const{ return m_iBlockVersion; }
	int getBlockInt();
	char getBlockChar();
	bool blockDone() const{ return (m_iBlockPointer >= m_iBlockSize); }

	bool isFileDone() const{ return m_bFileDone; }

	void writeBegin(unsigned int version);
	void writeEnd();
	void writeBlockInt(int);
	void writeBlockChar(char);

	const char *blockID() const{ return m_cBlockID; }
	bool readBlock();
	void getBlock(void *buf, unsigned int size);
	void writeBlock(const void *data, unsigned int size);
	bool flushBlock();
	void createBlock(const char *id, int version);

	std::string readString();
	void writeString(const char *s);
	void writeString(const std::string &s);

	void rollbackPointer(int count);	// avoid this
private:
	unsigned int m_iFileVersion;

	core::IO * m_io;

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

