#ifndef _THREADS_HPP_
#define _THREADS_HPP_

class SoundGen;

namespace boost
{
	class thread;
}

namespace gui
{
	class SoundGenThread
	{
	public:
		SoundGenThread(SoundGen *gen);
		~SoundGenThread();
		void run();
		void wait();
		bool isRunning();
	private:
		SoundGen *m_gen;
		boost::thread *m_thread;
		volatile bool m_thread_running;

		void _delthread();
		static void _job(SoundGenThread *t);
	};
}

#endif

