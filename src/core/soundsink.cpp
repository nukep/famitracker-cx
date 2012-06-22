#include <stdio.h>
#include <dlfcn.h>
#include <map>
#include <string>
#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>
#include <unistd.h>
#include "soundsink.hpp"

namespace core
{
	struct _soundsink_threading_t
	{
		boost::thread *t;
		boost::mutex mtx_playing;
		boost::condition cond_playing;
		volatile bool running;

		void delthread()
		{
			delete t;
			t = NULL;
		}
	};

	SoundSink::SoundSink()
		: m_curtimeidxbuf(0), m_timeidxsz(0), m_playing(false)
	{
		m_threading = new _soundsink_threading_t;
		m_threading->t = NULL;
		m_threading->running = false;
	}
	SoundSink::~SoundSink()
	{
		if (m_threading->running)
		{
			m_threading->t->join();
		}
		if (m_threading != NULL)
		{
			m_threading->delthread();
		}

		delete m_threading;
	}
	void SoundSink::setPlaying(bool playing)
	{
		{
			boost::lock_guard<boost::mutex> lock(m_threading->mtx_playing);
			m_playing = playing;
		}
		m_threading->cond_playing.notify_one();
	}

	struct timestamp_t
	{
		struct timeval tv;
		void gettime()
		{
			gettimeofday(&tv, NULL);
		}
		int diff_ms(const timestamp_t &before) const
		{
			int s = tv.tv_sec - before.tv.tv_sec;
			int m = (tv.tv_usec - before.tv.tv_usec)/1000;

			return s*1000 + m;
		}
		bool isLessThan(const timestamp_t &before) const
		{
			if (tv.tv_sec < before.tv.tv_sec)
				return true;
			if (tv.tv_sec > before.tv.tv_sec)
				return false;
			if (tv.tv_usec < before.tv.tv_usec)
				return true;

			return false;
		}
		timestamp_t add_ms(unsigned int ms) const
		{
			timestamp_t t = *this;
			t.tv.tv_usec += ms*1000;
			t.tv.tv_sec += t.tv.tv_usec/1000000;
			t.tv.tv_usec %= 1000000;
			return t;
		}
	};

	static unsigned int ms_from_idx(core::u32 v, core::u32 sr)
	{
		return v * 1000 / sr;
	}

	static void timesync(timestamp_t begin, core::u32 *idx, core::u32 sz, core::u32 sr, SoundSink::time_callback_t cb, void *data, _soundsink_threading_t *th)
	{
		bool immediate_update = false;
		core::u32 skip = 0;

		for (core::u32 i = 0; i < sz;)
		{
			timestamp_t tgt = begin.add_ms(ms_from_idx(idx[i], sr));
			timestamp_t cur;
			cur.gettime();
			if (tgt.isLessThan(cur))
			{
				immediate_update = true;
				skip++;
				i++;
				continue;
			}

			if (immediate_update)
			{
				immediate_update = false;
				(*cb)(skip, data);
				skip = 0;
				continue;
			}

			usleep(tgt.diff_ms(cur)*1000);

			(*cb)(1, data);
			i++;
		}
		if (immediate_update)
		{
			(*cb)(skip, data);
		}

		th->running = false;
	}

	void SoundSink::performSoundCallback(s16 *buf, u32 sz)
	{
		core::u32 timec = (*m_soundCallback)(buf, sz, m_callbackData, m_timeidx[m_curtimeidxbuf]);

		m_timeidxsz = timec;
	}

	void SoundSink::performTimeCallback()
	{
		if (m_timeidxsz == 0)
			return;

		// start a thread that waits the appropriate times and
		// dispatches the callbacks

		timestamp_t ts;
		ts.gettime();

		core::u32 sz = m_timeidxsz;
		if (sz > MAX_TIMEIDX)
			sz = MAX_TIMEIDX;

		if (m_threading->running)
		{
			// probably still running from late timestamp
			// we'll just have to wait, then
			m_threading->t->join();
		}
		if (m_threading != NULL)
		{
			m_threading->delthread();
		}

		m_threading->running = true;
		m_threading->t = new boost::thread(timesync, ts, m_timeidx[m_curtimeidxbuf], m_timeidxsz, sampleRate(), m_timeCallback, m_callbackData, m_threading);

		// swap buffers
		m_curtimeidxbuf = (m_curtimeidxbuf == 0) ? 1 : 0;
		m_timeidxsz = 0;
	}

	void SoundSink::blockUntilStopped()
	{
		boost::unique_lock<boost::mutex> lock(m_threading->mtx_playing);
		while (m_playing)
		{
			m_threading->cond_playing.wait(lock);
		}
	}

	struct sound_handle_t
	{
		typedef core::SoundSink*(*create_f)();
		void *handle;
		create_f create;
	};

	typedef std::map<std::string, sound_handle_t> LoadedSoundSinksMap;
	static LoadedSoundSinksMap loaded_sound_sinks;

	core::SoundSink * loadSoundSink(const char *name)
	{
		LoadedSoundSinksMap::const_iterator it = loaded_sound_sinks.find(name);
		if (it == loaded_sound_sinks.end())
		{
			// not found
			sound_handle_t h;
			char libname[128];
			sprintf(libname, "libcore-%s-sound.so", name);
			h.handle = dlopen(libname, RTLD_LAZY);
			if (h.handle == NULL)
			{
				fprintf(stderr, "loadSoundSink: Could not load: %s\n", dlerror());
				return NULL;
			}

			h.create = (sound_handle_t::create_f)dlsym(h.handle, "sound_create");
			if (h.create == NULL)
			{
				fprintf(stderr, "loadSoundSink: %s\n", dlerror());
				return NULL;
			}
			loaded_sound_sinks[name] = h;

			return (*h.create)();
		}
		else
		{
			const sound_handle_t &h = it->second;
			return (*h.create)();
		}
	}
}
