#ifndef _GUI_THREADPOOL_HPP_
#define _GUI_THREADPOOL_HPP_

#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>
#include <queue>
#include "GUI_App.hpp"

namespace gui
{
	class App;

	enum
	{
		T_PLAYSONG, T_STOPSONG, T_STOPSONGTRACKER,
		T_AUDITION, T_HALTAUDITION,
		T_DELETESINK, T_TERMINATE
	};

	struct threadpool_playing_task
	{
		threadpool_playing_task(){}
		threadpool_playing_task(int task)
			: m_task(task), m_cb(NULL), m_cb_data(NULL)
		{
		}
		threadpool_playing_task(int task, mainthread_callback_t cb, void *cb_data)
			: m_task(task), m_cb(cb), m_cb_data(cb_data)
		{
		}

		int m_task;
		mainthread_callback_t m_cb;
		void * m_cb_data;

		struct audition_t
		{
			bool playrow;
			int octave, note, inst, channel;
		};
		struct playing_t
		{
			bool startatrow0;
		};

		union
		{
			playing_t m_playing;
			audition_t m_audition;
		};
	};

	class ThreadPool
	{
	public:
		ThreadPool(App *a);
		~ThreadPool();

		void threadpool_playing_post(const threadpool_playing_task &t);
		App * app() const{ return m_app; }
	private:
		App *m_app;

		boost::thread *thread_threadpool_playing;
		boost::mutex mtx_threadpool_playing;
		boost::condition cond_threadpool_playing;
		std::queue<threadpool_playing_task> threadpool_playing_queue;

		static void func_playing_bootstrap(ThreadPool *tp);
		void func_playing();
		void playsong_thread(mainthread_callback_t cb, void *data, const threadpool_playing_task::playing_t &p);
		void stopsong_thread(mainthread_callback_t cb, void *data);
		void stopsongtracker_thread(mainthread_callback_t cb, void *data);
		void auditionnote_thread(const threadpool_playing_task::audition_t &a);
		void auditionhalt_thread();
		void deletesink_thread(mainthread_callback_t cb, void *data);
	};
}

#endif

