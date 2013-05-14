#ifndef _GUI_SOUND_HPP_
#define _GUI_SOUND_HPP_

#include "famitracker-core/SoundGen.hpp"
#include "core/soundsink.hpp"
#include "core/threadpool.hpp"
#include <string>

namespace boost
{
	class thread;
}

namespace gui
{
	class App;

	class SoundConfiguration
	{
	public:
		SoundConfiguration();
		void setFreq(unsigned int freq)
		{
			m_freq = freq;
		}

		void setLatency(unsigned int ms)
		{
			m_latency_ms = ms;
		}

		void setEngine(const std::string engine)
		{
			m_engine = engine;
		}

		void setTrackerUpdate(SoundGen::trackerupdate_f f, void *data)
		{
			m_trackerUpdate = f;
			m_trackerUpdateData = data;
		}

		void setDocument(FtmDocument *doc)
		{
			m_doc = doc;
		}

		unsigned int freq() const{ return m_freq; }
		unsigned int latency_ms() const{ return m_latency_ms; }
		const std::string & engine() const{ return m_engine; }
		SoundGen::trackerupdate_f trackerUpdate() const{ return m_trackerUpdate; }
		void * trackerUpdateData() const{ return m_trackerUpdateData; }
		FtmDocument * document() const{ return m_doc; }
	private:
		unsigned int m_freq, m_latency_ms;
		std::string m_engine;

		SoundGen::trackerupdate_f m_trackerUpdate;
		void *m_trackerUpdateData;

		FtmDocument *m_doc;
	};

	/*
		The gui::Sound class is responsible for interfacing with the underlying
		sound engine.
	*/
	class Sound
	{
	public:
		Sound();
		~Sound();

		void reload(const SoundConfiguration &conf);
		void reload();
		void setIsPlaying(bool playing);
		const SoundConfiguration & configuration(){ return m_conf; }

		void setMuted(unsigned int channel, bool mute);
		bool isMuted(unsigned int channel);
	private:
		App * m_app;

		SoundConfiguration m_conf;

		SoundGen *m_sgen;
		core::SoundSinkPlayback *m_sink;

		core::threadpool::Queue m_tp;
		boost::thread * m_tpThread;

		void unload();
	};
}

#endif

