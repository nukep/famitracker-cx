#ifndef SOUNDTHREAD_HPP
#define SOUNDTHREAD_HPP

#include "core/common.hpp"

namespace boost
{
	class thread;
	class mutex;
}

class SoundThread
{
public:
	typedef void (*callback_t)(void*);
	SoundThread();
	~SoundThread();
	void run(callback_t f, void *data);
	void wait();
private:
	boost::thread *m_thread;
	boost::mutex m_mtx_running;
	bool m_running;

	void _wait_nomtx();
	static void _job(SoundThread *t, callback_t f, void *data);
	void _delthread();
};

#endif

