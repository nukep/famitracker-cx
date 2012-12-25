#include <string.h>
#include "Document.hpp"
#include "common.hpp"

const unsigned int MAX_BLOCK_SIZE = 0x80000;
const unsigned int BLOCK_SIZE = 0x10000;
const char FILE_HEADER_ID[] = "FamiTracker Module";
const char FILE_END_ID[] = "END";

Document::Document()
	: m_pBlockData(NULL), m_bFileDone(false), m_io(NULL)
{
}

Document::~Document()
{
	if (m_pBlockData != NULL)
		delete[] m_pBlockData;
}

bool Document::checkValidity()
{
	char buf[256];
	if (!m_io->read_e(buf, sizeof(FILE_HEADER_ID)-1))
	{
		return false;
	}

	if (memcmp(buf, FILE_HEADER_ID, sizeof(FILE_HEADER_ID)-1) != 0)
	{
		return false;
	}

	// Read file version
	if (!m_io->readInt(&m_iFileVersion))
	{
		return false;
	}
	return true;
}

bool Document::readBlock()
{
	m_iBlockPointer = 0;
	memset(m_cBlockID, 0, 16);

	Quantity bytesRead = m_io->read(m_cBlockID, 16);

	if (strcmp(m_cBlockID, FILE_END_ID) == 0)
	{
		m_bFileDone = true;
		return true;
	}

	bool read_v = m_io->readInt(&m_iBlockVersion);
	bool read_s = m_io->readInt(&m_iBlockSize);
	if (!read_v || !read_s)
	{
		return false;
	}

	if (m_iBlockSize > 50000000)
	{
		// File is probably corrupt
		memset(m_cBlockID, 0, 16);
		return false;
	}

	init_pBlockData(m_iBlockSize);
	if (!m_io->read_e(m_pBlockData, m_iBlockSize))
	{
		return false;
	}

	if (bytesRead == 0)
		m_bFileDone = true;

	return true;
}

void Document::getBlock(void *buf, unsigned int size)
{
	memcpy(buf, m_pBlockData + m_iBlockPointer, size);
	m_iBlockPointer += size;
}

void Document::writeBlock(const void *data, unsigned int size)
{
	ftkr_Assert(m_pBlockData != NULL);

	unsigned int writeSize, writePtr = 0;

	const char *d = (const char*)data;

	// Allow block to grow in size
	while (size > 0)
	{
		if (size > BLOCK_SIZE)
			writeSize = BLOCK_SIZE;
		else
			writeSize = size;

		if ((m_iBlockPointer + writeSize) >= m_iMaxBlockSize)
			reallocateBlock();

		memcpy(m_pBlockData + m_iBlockPointer, d + writePtr, writeSize);
		m_iBlockPointer += writeSize;
		size -= writeSize;
		writePtr += writeSize;
	}
}

bool Document::flushBlock()
{
	if (m_pBlockData == NULL)
		return false;

	m_io->write(m_cBlockID, 16);
	m_io->writeInt(m_iBlockVersion);
	m_io->writeInt(m_iBlockPointer);
	m_io->write(m_pBlockData, m_iBlockPointer);
	return true;
}

void Document::writeBegin(unsigned int version)
{
	m_iFileVersion = version;

	m_io->write(FILE_HEADER_ID, sizeof(FILE_HEADER_ID)-1);
	m_io->writeInt(m_iFileVersion);
}
void Document::writeEnd()
{
	m_io->write(FILE_END_ID, sizeof(FILE_END_ID)-1);
}

void Document::writeBlockInt(int v)
{
	unsigned char d[4];
	d[0] = v & 0xFF;
	d[1] = (v>>8) & 0xFF;
	d[2] = (v>>16) & 0xFF;
	d[3] = (v>>24) & 0xFF;
	writeBlock(d, 4);
}

void Document::writeBlockChar(char v)
{
	writeBlock(&v, 1);
}

void Document::createBlock(const char *id, int version)
{
	memset(m_cBlockID, 0, 16);
	safe_strcpy(m_cBlockID, id, sizeof(m_cBlockID));

	m_iBlockPointer = 0;
	m_iBlockSize = 0;
	m_iBlockVersion = version & 0xFFFF;
	m_iMaxBlockSize = BLOCK_SIZE;

	init_pBlockData(m_iMaxBlockSize);
}

std::string Document::readString()
{
	std::string s;
	char c;

	while ((c = getBlockChar()) != 0)
		s.push_back(c);

	return s;
}

void Document::writeString(const char *s)
{
	size_t sz = strlen(s);
	writeBlock(s, sz);
	writeBlockChar(0);
}

void Document::writeString(const std::string &s)
{
	size_t sz = s.size();
	writeBlock(s.c_str(), sz);
	writeBlockChar(0);
}

void Document::rollbackPointer(int count)
{
	m_iBlockPointer -= count;
}

void Document::init_pBlockData(Quantity size)
{
	if (m_pBlockData != NULL)
		delete[] m_pBlockData;

	m_pBlockData = new char[size];
}
void Document::reallocateBlock()
{
	ftkr_Assert(m_pBlockData != NULL);

	unsigned int oldSize = m_iMaxBlockSize;
	m_iMaxBlockSize += BLOCK_SIZE;
	char *p = new char[m_iMaxBlockSize];
	memcpy(p, m_pBlockData, oldSize);

	delete[] m_pBlockData;
	m_pBlockData = p;
}

int Document::getBlockInt()
{
	const unsigned char *buf = (const unsigned char*)m_pBlockData + m_iBlockPointer;
	int Value = (buf[3] << 24) | (buf[2] << 16) | (buf[1] << 8) | buf[0];
	m_iBlockPointer += 4;
	return Value;
}

char Document::getBlockChar()
{
	const char *buf = m_pBlockData + m_iBlockPointer;
	char Value = buf[0];
	m_iBlockPointer += 1;
	return Value;
}
