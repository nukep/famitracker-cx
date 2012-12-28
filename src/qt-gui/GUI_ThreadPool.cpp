#include "GUI.hpp"
#include "GUI_ThreadPool.hpp"
#include "GUI_App.hpp"
#include "famitracker-core/TrackerController.hpp"
#include "MainWindow.hpp"

namespace gui
{
	ThreadPool::ThreadPool(App *a)
		: m_app(a)
	{
		thread_threadpool_playing = new boost::thread(func_playing_bootstrap, this);
	}
	ThreadPool::~ThreadPool()
	{
		threadpool_playing_post(threadpool_playing_task(T_TERMINATE));
		thread_threadpool_playing->join();
		delete thread_threadpool_playing;
	}
	void ThreadPool::threadpool_playing_post(const threadpool_playing_task &t)
	{
		mtx_threadpool_playing.lock();
		threadpool_playing_queue.push(t);
		cond_threadpool_playing.notify_all();
		mtx_threadpool_playing.unlock();
	}
	void ThreadPool::func_playing_bootstrap(ThreadPool *tp)
	{
		tp->func_playing();
	}

	void ThreadPool::playsong_thread(mainthread_callback_t cb, void *data, const threadpool_playing_task::playing_t &p)
	{
		const DocInfo *dinfo = m_app->activeDocInfo();

		bool startatrow0 = p.startatrow0;

		int row = startatrow0?0:dinfo->currentRow();

		m_app->sgen->trackerController()->startAt(dinfo->currentFrame(), row);

		m_app->sgen->startTracker();

		m_app->mtx_is_playing.lock();
		m_app->is_playing = true;
		m_app->mtx_is_playing.unlock();

		if (m_app->mw != NULL)
		{
			m_app->mw->sendIsPlayingSongEvent(cb, data, true);
		}
	}

	void ThreadPool::stopsong_thread(mainthread_callback_t cb, void *data)
	{
		m_app->sgen->stopTracker();
		m_app->sink->blockUntilStopped();
		m_app->sink->blockUntilTimerEmpty();

		m_app->mtx_is_playing.lock();
		m_app->is_playing = false;
		m_app->mtx_is_playing.unlock();

		if (m_app->mw != NULL)
		{
			m_app->mw->sendIsPlayingSongEvent(cb, data, false);
		}
	}

	void ThreadPool::stopsongtracker_thread(mainthread_callback_t cb, void *data)
	{
		m_app->sgen->stopTracker();
		m_app->sgen->blockUntilTrackerStopped();

		m_app->mtx_is_playing.lock();
		m_app->is_playing = false;
		m_app->mtx_is_playing.unlock();

		if (m_app->mw != NULL)
		{
			m_app->mw->sendIsPlayingSongEvent(cb, data, false);
		}
	}

	void ThreadPool::auditionnote_thread(const threadpool_playing_task::audition_t &a)
	{
		if (a.playrow)
		{
			const DocInfo *dinfo = m_app->activeDocInfo();
			m_app->sgen->auditionRow(dinfo->currentFrame(), dinfo->currentRow());
		}
		else
		{
			m_app->sgen->auditionNote(a.note, a.octave, a.inst, a.channel);
		}
	}

	void ThreadPool::auditionhalt_thread()
	{
		m_app->sgen->auditionHalt();
	}

	void ThreadPool::deletesink_thread(mainthread_callback_t cb, void *data)
	{
		delete m_app->sink;
		m_app->sink = NULL;

		if (m_app->mw != NULL)
		{
			m_app->mw->sendIsPlayingSongEvent(cb, data, isPlaying());
		}
	}

	void ThreadPool::func_playing()
	{
		bool terminate = false;
		do
		{
			threadpool_playing_task t;

			mtx_threadpool_playing.lock();
			bool empty;
			do
			{
				empty = threadpool_playing_queue.empty();
				if (!empty)
				{
					// FIFO
					t = threadpool_playing_queue.front();
					threadpool_playing_queue.pop();			// pop from front
				}
				else
				{
					// wait until queue is filled
					cond_threadpool_playing.wait(mtx_threadpool_playing);
				}
			}
			while (empty);
			mtx_threadpool_playing.unlock();

			// do what's on the queue

			switch (t.m_task)
			{
			case T_PLAYSONG:
				playsong_thread(t.m_cb, t.m_cb_data, t.m_playing);
				break;
			case T_STOPSONG:
				stopsong_thread(t.m_cb, t.m_cb_data);
				break;
			case T_STOPSONGTRACKER:
				stopsongtracker_thread(t.m_cb, t.m_cb_data);
				break;
			case T_AUDITION:
				auditionnote_thread(t.m_audition);
				break;
			case T_HALTAUDITION:
				auditionhalt_thread();
				break;

			case T_DELETESINK:
				deletesink_thread(t.m_cb, t.m_cb_data);
				break;
			// gracefully end the thread pool
			case T_TERMINATE:
				terminate = true;
				break;

			default:	// ignore
				break;
			}
		}
		while (!terminate);
	}
}
