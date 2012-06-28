#include <boost/thread.hpp>
#include "soundthread.hpp"

namespace core
{
	SoundThread::SoundThread()
		: m_thread(NULL), m_running(false)
	{
	}
	SoundThread::~SoundThread()
	{
		wait();
	}

	void SoundThread::run(callback_t f, void *data)
	{
		if (m_thread != NULL)
		{
			if (!m_running)
				_delthread();
			else
				return;
		}

		m_running = true;
		m_thread = new boost::thread(_job, this, f, data);
	}
	void SoundThread::wait()
	{
		if (m_thread == NULL)
			return;

		if (m_running)
		{
			if (m_thread->get_id() == boost::this_thread::get_id())
			{
				// waiting on the current thread will deadlock
				return;
			}

			m_thread->join();
		}
		_delthread();
	}

	void SoundThread::_job(SoundThread *t, callback_t f, void *data)
	{
		(*f)(data);
		t->m_running = false;
	}

	void SoundThread::_delthread()
	{
		delete m_thread;
		m_thread = NULL;
	}
}
