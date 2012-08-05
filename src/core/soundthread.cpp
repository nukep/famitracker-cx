#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>
#include "soundthread.hpp"

namespace core
{
	SoundThread::SoundThread()
		: m_thread(NULL), m_running(false)
	{
		m_mtx_running = new boost::mutex;
	}
	SoundThread::~SoundThread()
	{
		wait();
		delete m_mtx_running;
	}

	void SoundThread::run(callback_t f, void *data)
	{
		m_mtx_running->lock();
		if (m_thread != NULL)
		{
			if (m_running)
			{
				_wait_nomtx();
			}
			_delthread();
		}

		m_running = true;
		m_thread = new boost::thread(_job, this, f, data);
		m_mtx_running->unlock();
	}
	void SoundThread::wait()
	{
		boost::lock_guard<boost::mutex> lock(*m_mtx_running);
		_wait_nomtx();
	}
	void SoundThread::_wait_nomtx()
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

			// unlock the mutex because _job acquires it
			m_mtx_running->unlock();
			m_thread->join();
			m_mtx_running->lock();
		}
		_delthread();
	}

	void SoundThread::_job(SoundThread *t, callback_t f, void *data)
	{
		(*f)(data);
		t->m_mtx_running->lock();
		t->m_running = false;
		t->m_mtx_running->unlock();
	}

	void SoundThread::_delthread()
	{
		delete m_thread;
		m_thread = NULL;
	}
}
