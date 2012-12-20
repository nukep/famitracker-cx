#ifndef _WAVOUTPUT_HPP_
#define _WAVOUTPUT_HPP_

#include "Document.hpp"
#include "core/soundsink.hpp"

class WavOutput : public core::SoundSinkExport
{
public:
	WavOutput(core::IO *io, int channels, int sampleRate);

	void flushBuffer(core::s16 *Buffer, core::u32 Size);
	void flush(){}

	void finalize();

	int sampleRate() const{ return m_sampleRate; }
private:
	core::IO *m_io;
	Quantity m_size;

	int m_sampleRate;
};

#endif
