#include <stdio.h>
#include <dlfcn.h>
#include <map>
#include <string>
#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>
#include "soundsink.hpp"
#include "time.hpp"

namespace core
{
	struct _soundsink_threading_t
	{
		boost::thread *t;
		boost::mutex mtx_playing;
		boost::condition cond_playing;
		volatile bool running;
		volatile bool destructing;

		boost::mutex mtx_time_ringbuffer;
		boost::condition cond_time_ringbuffer;

		void delthread()
		{
			delete t;
			t = NULL;
		}
	};

	SoundSink::SoundSink()
		: m_timeidxsz(0), m_playing(false),
		  m_timeidx_ringbuffer(sizeof(core::timestamp_t))
	{
		// give the ring buffer a generous amount of memory
		m_timeidx_ringbuffer.resize(MAX_TIMEIDX*16);

		m_threading = new _soundsink_threading_t;
		m_threading->destructing = false;
		m_threading->running = true;
		m_threading->t = new boost::thread(_timeloop_bootstrap, this);
	}
	SoundSink::~SoundSink()
	{
		if (m_threading->running)
		{
			m_threading->mtx_time_ringbuffer.lock();
			m_threading->destructing = true;
			m_threading->cond_time_ringbuffer.notify_all();
			m_threading->mtx_time_ringbuffer.unlock();
			m_threading->t->join();
		}
		if (m_threading != NULL)
		{
			m_threading->delthread();
		}
		setPlaying(false);

		delete m_threading;
	}
	void SoundSink::setPlaying(bool playing)
	{
		{
			boost::lock_guard<boost::mutex>(m_threading->mtx_playing);

			bool changed = playing != m_playing;
			if (!changed)
				return;

			m_playing = playing;

			if (!playing)
			{
				// wait for the timing thread to finish the ring buffer
				blockUntilTimerEmpty();
			}
		}
		m_threading->cond_playing.notify_one();
	}

	static inline core::u32 us_from_idx(core::u32 v, core::u32 sr)
	{
		core::u64 a = v;
		a = a * 1000000 / sr;
		return a;
	}

	static inline core::u32 ms_from_idx(core::u32 v, core::u32 sr)
	{
		return v * 1000 / sr;
	}

	// return true if the loop should end
	bool SoundSink::_timeloop_read(timestamp_t &tgt, core::u32 &skip)
	{
		boost::unique_lock<boost::mutex> lock(m_threading->mtx_time_ringbuffer);
		while (m_timeidx_ringbuffer.isEmpty())
		{
			// notify that the ringbuffer is empty
			// (this shouldn't affect the wait we have shortly after)
			m_threading->cond_time_ringbuffer.notify_all();

			if (skip > 0)
			{
				// perform skip callbacks before waiting
				(*m_timeCallback)(skip, m_callbackData);
				skip = 0;
			}

			// SoundSink could be destructing. let's check
			if (m_threading->destructing)
			{
				// the thread has to finish
				return true;
			}

			// wait until the ring buffer is filled
			m_threading->cond_time_ringbuffer.wait(lock);
		}
		m_timeidx_ringbuffer.read(&tgt, 1);
		return false;
	}

	void SoundSink::_timeloop()
	{
		core::u32 skip = 0;

		while (true)
		{
			timestamp_t tgt;

			if (_timeloop_read(tgt, skip))
			{
				break;
			}

compare_time:

			timestamp_t cur;
			cur.gettime();

			if (tgt.isLessThan(cur))
			{
				// the timestamp has already elapsed. skip it
				skip++;
				continue;
			}
			if (skip > 0)
			{
				(*m_timeCallback)(skip, m_callbackData);
				skip = 0;
				goto compare_time;
			}

			core::sleep_us(tgt.diff_us(cur));

			(*m_timeCallback)(1, m_callbackData);
		}

		if (skip > 0)
		{
			(*m_timeCallback)(skip, m_callbackData);
		}

		m_threading->running = false;
	}

	void SoundSink::_timeloop_bootstrap(SoundSink *s)
	{
		s->_timeloop();
	}

	void SoundSink::performSoundCallback(s16 *buf, u32 sz)
	{
		core::u32 timec = (*m_soundCallback)(buf, sz, m_callbackData, m_timeidx);

		m_timeidxsz = timec;
	}

	void SoundSink::applyTime(core::s32 delay_us)
	{
		if (m_timeidxsz == 0)
			return;

		core::timestamp_t now;
		now.gettime();

		core::u32 sr = sampleRate();
		core::timestamp_t arr[MAX_TIMEIDX];
		for (Quantity i = 0; i < m_timeidxsz; i++)
		{
			core::timestamp_t ts = now;
			core::s32 delta = delay_us + (core::s32)us_from_idx(m_timeidx[i], sr);
			if (delta > 0)
			{
				ts = ts.add_us(delta);
			}
			arr[i] = ts;
		}

		{
			boost::lock_guard<boost::mutex>(m_threading->mtx_time_ringbuffer);

			if (m_timeidx_ringbuffer.write(arr, m_timeidxsz) < m_timeidxsz)
			{
				fprintf(stderr, "m_timeidx_ringbuffer overrun\n");
				// buffer overrun
			}
		}
		// in case the timer thread is waiting on the ring buffer, signal the thread

		m_threading->cond_time_ringbuffer.notify_all();
	}
/*
	void SoundSink::performTimeCallback()
	{
		m_timeidxsz = 0;
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
*/
	void SoundSink::blockUntilStopped()
	{
		boost::unique_lock<boost::mutex> lock(m_threading->mtx_playing);
		while (m_playing)
		{
			m_threading->cond_playing.wait(lock);
		}
	}
	void SoundSink::blockUntilTimerEmpty()
	{
		// blocks until timer ringbuffer is empty
		// also guarantees the timer callback isn't running
		// this should be called when applyTime calls are halted.
		// (it's possible for this to unblock while the sound sink is active, as
		// the time loop may temporarily run out of times as it's waiting for them)
		boost::unique_lock<boost::mutex> lock(m_threading->mtx_time_ringbuffer);
		while (!m_timeidx_ringbuffer.isEmpty())
		{
			// wait until the ring buffer is empty
			m_threading->cond_time_ringbuffer.wait(lock);
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
