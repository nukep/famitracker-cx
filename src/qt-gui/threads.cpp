#include <boost/thread.hpp>
#include "threads.hpp"
#include "core/sound.hpp"

namespace gui
{
	SoundGenThread::SoundGenThread(SoundGen *gen)
		: m_gen(gen), m_thread(NULL), m_thread_running(false)
	{
	}
	SoundGenThread::~SoundGenThread()
	{
		wait();
	}

	void SoundGenThread::run()
	{
		if (m_thread != NULL)
			return;

		m_thread_running = true;
		m_thread = new boost::thread(_job, this);
	}
	void SoundGenThread::wait()
	{
		if (m_thread == NULL)
			return;

		m_thread->join();
		_delthread();
	}

	bool SoundGenThread::isRunning()
	{
		if (m_thread == NULL)
			return false;

		// don't worry about mutexes - it's a flag
		if (!m_thread_running)
		{
			_delthread();
			return false;
		}

		return true;
	}

	void SoundGenThread::_delthread()
	{
		delete m_thread;
		m_thread = NULL;
	}

	void SoundGenThread::_job(SoundGenThread *t)
	{
		t->m_gen->run();
		t->m_thread_running = false;
	}
}
