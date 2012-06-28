#ifndef CORE_SOUNDTHREAD_HPP
#define CORE_SOUNDTHREAD_HPP

namespace boost
{
	class thread;
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
		volatile bool m_running;

		static void _job(SoundThread *t, callback_t f, void *data);
		void _delthread();
	};
}

#endif

