#ifndef CORE_SOUNDSINK_HPP
#define CORE_SOUNDSINK_HPP

#include "types.hpp"

namespace core
{
	class SoundSink
	{
	public:
		virtual ~SoundSink(){}
		virtual void flush() = 0;
		virtual int sampleRate() const = 0;
		virtual void flushBuffer(core::s16 *buffer, core::u32 size) = 0;
	};

	class SoundSinkPlayback : public SoundSink
	{
	public:
		virtual void initialize(unsigned int sampleRate, unsigned int channels, unsigned int latency_ms) = 0;
		virtual void close() = 0;
	};

	core::SoundSink * loadSoundSink(const char *name);
}

#endif
