#ifndef CORE_SOUNDTHREAD_HPP
#define CORE_SOUNDTHREAD_HPP

namespace boost
{
	class thread;
	class mutex;
}

namespace core
{
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
		boost::mutex *m_mtx_running;
		volatile bool m_running;

		void _wait_nomtx();
		static void _job(SoundThread *t, callback_t f, void *data);
		void _delthread();
	};
}

#endif

