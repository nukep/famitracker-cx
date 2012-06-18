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

	bool checkValidity(core::IO *io);

	unsigned int getFileVersion() const{ return m_iFileVersion; }

	unsigned int getBlockVersion() const{ return m_iBlockVersion; }
	int getBlockInt();
	char getBlockChar();
	bool blockDone() const{ return (m_iBlockPointer >= m_iBlockSize); }

	bool isFileDone() const{ return m_bFileDone; }

	void writeBlockInt(int);
	void writeBlockChar(char);

	const char *blockID() const{ return m_cBlockID; }
	bool readBlock(core::IO *io);
	void getBlock(void *buf, unsigned int size);
	void writeBlock(const void *data, unsigned int size);
	void createBlock(const char *id, int version);

	std::string readString();

	void rollbackPointer(int count);	// avoid this
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

