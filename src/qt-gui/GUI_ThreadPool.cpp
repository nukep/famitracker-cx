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

	static void playsong_thread(App *app, mainthread_callback_t cb, void *data, const threadpool_playing_task::playing_t &p)
	{
		const DocInfo *dinfo = activeDocInfo();

		bool startatrow0 = p.startatrow0;

		int row = startatrow0?0:dinfo->currentRow();

		app->sgen->trackerController()->startAt(dinfo->currentFrame(), row);

		app->sgen->startTracker();

		app->mtx_is_playing.lock();
		app->is_playing = true;
		app->mtx_is_playing.unlock();

		if (app->mw != NULL)
		{
			app->mw->sendIsPlayingSongEvent(cb, data, true);
		}
	}

	static void stopsong_thread(App *app, mainthread_callback_t cb, void *data)
	{
		app->sgen->stopTracker();
		app->sink->blockUntilStopped();
		app->sink->blockUntilTimerEmpty();

		app->mtx_is_playing.lock();
		app->is_playing = false;
		app->mtx_is_playing.unlock();

		if (app->mw != NULL)
		{
			app->mw->sendIsPlayingSongEvent(cb, data, false);
		}
	}

	static void stopsongtracker_thread(App *app, mainthread_callback_t cb, void *data)
	{
		app->sgen->stopTracker();
		app->sgen->blockUntilTrackerStopped();

		app->mtx_is_playing.lock();
		app->is_playing = false;
		app->mtx_is_playing.unlock();

		if (app->mw != NULL)
		{
			app->mw->sendIsPlayingSongEvent(cb, data, false);
		}
	}

	static void auditionnote_thread(App *app, const threadpool_playing_task::audition_t &a)
	{
		if (a.playrow)
		{
			DocInfo *dinfo = activeDocInfo();
			app->sgen->auditionRow(dinfo->currentFrame(), dinfo->currentRow());
		}
		else
		{
			app->sgen->auditionNote(a.note, a.octave, a.inst, a.channel);
		}
	}

	static void auditionhalt_thread(App *app)
	{
		app->sgen->auditionHalt();
	}

	static void deletesink_thread(App *app, mainthread_callback_t cb, void *data)
	{
		delete app->sink;
		app->sink = NULL;

		if (app->mw != NULL)
		{
			app->mw->sendIsPlayingSongEvent(cb, data, isPlaying());
		}
	}

	void ThreadPool::func_playing()
	{
		App *app = m_app;

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
					t = threadpool_playing_queue.front();
					threadpool_playing_queue.pop();
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
				playsong_thread(app, t.m_cb, t.m_cb_data, t.m_playing);
				break;
			case T_STOPSONG:
				stopsong_thread(app, t.m_cb, t.m_cb_data);
				break;
			case T_STOPSONGTRACKER:
				stopsongtracker_thread(app, t.m_cb, t.m_cb_data);
				break;
			case T_AUDITION:
				auditionnote_thread(app, t.m_audition);
				break;
			case T_HALTAUDITION:
				auditionhalt_thread(app);
				break;

			case T_DELETESINK:
				deletesink_thread(app, t.m_cb, t.m_cb_data);
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
