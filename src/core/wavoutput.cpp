#include "wavoutput.hpp"

WavOutput::WavOutput(IO *io, int chans, int sampleRate)
	: m_io(io), m_size(0), m_sampleRate(sampleRate)
{
	int bpsmp = 2;	// bytes per sample (per channel)

	io->write("RIFF", 4);
	io->writeInt(0);						// file size - 8
	io->write("WAVE", 4);

	io->write("fmt ", 4);
	io->writeInt(16);						// extra format bytes + 16
	io->writeShort(1);						// compression (1=PCM/uncompressed)
	io->writeShort(chans);					// number of channels
	io->writeInt(sampleRate);				// sample rate
	io->writeInt(bpsmp*chans*sampleRate);	// bytes per second
	io->writeShort(bpsmp*chans);			// block align
	io->writeShort(bpsmp*8);				// significant bits per sample

	io->write("data", 4);
	io->writeInt(0);						// size of following data
}

void WavOutput::FlushBuffer(int16 *Buffer, uint32 Size)
{
	m_io->write(Buffer, Size*2);
	m_size += Size*2;
}

void WavOutput::finalize()
{
	// write final size
	m_io->seek(4, IO_SEEK_SET);
	m_io->writeInt(m_size+44 - 8);

	// write data size
	m_io->seek(40, IO_SEEK_SET);
	m_io->writeInt(m_size);
}
