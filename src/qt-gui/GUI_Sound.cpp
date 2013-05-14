#include "GUI_Sound.hpp"
#include "famitracker-core/FtmDocument.hpp"
#include "famitracker-core/TrackerController.hpp"

namespace gui
{
	class Event : public core::threadpool::Event
	{

	};

	SoundConfiguration::SoundConfiguration()
		: m_freq(0), m_latency_ms(0), m_engine(),
		  m_trackerUpdate(NULL), m_trackerUpdateData(NULL),
		  m_doc(NULL)
	{
	}

	Sound::Sound()
		: m_sgen(NULL), m_sink(NULL)
	{

	}

	Sound::~Sound()
	{
		unload();
	}

	void Sound::reload(const SoundConfiguration &conf)
	{
		unload();

		m_conf = conf;

		m_sink = dynamic_cast<core::SoundSinkPlayback*>(core::loadSoundSink(conf.engine().c_str()));
		m_sink->initialize(conf.freq(), 1, conf.latency_ms());
		m_sgen = new SoundGen;
		m_sgen->setSoundSink(m_sink);
		m_sgen->setTrackerUpdate(conf.trackerUpdate(), conf.trackerUpdateData());
		m_sgen->setDocument(conf.document());
	}

	void Sound::reload()
	{
		reload(m_conf);
	}

	void Sound::unload()
	{
		if (m_sgen != NULL)
		{
			delete m_sgen;
			m_sgen = NULL;
		}
		if (m_sink != NULL)
		{
			delete m_sink;
			m_sink = NULL;
		}
	}

	void Sound::setMuted(unsigned int channel, bool mute)
	{
		m_conf.document()->lock();
		m_sgen->trackerController()->setMuted(channel, mute);
		m_conf.document()->unlock();
	}

	bool Sound::isMuted(unsigned int channel)
	{
		bool muted;

		m_conf.document()->lock();
		muted = m_sgen->trackerController()->muted(channel);
		m_conf.document()->unlock();

		return muted;
	}
}
