#include "threadpool.hpp"
#include <queue>
#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>

namespace core
{
	namespace threadpool
	{
		class BlockHandle
		{
		public:
			BlockHandle()
				: m_isDone(false), m_refCount(1)
			{
			}
			void block()
			{
				bool doDelete = false;

				m_mtx.lock();
				while (!m_isDone)
				{
					m_cond.wait(m_mtx);
				}

				m_refCount--;
				if (m_refCount == 0)
				{
					doDelete = true;
				}
				m_mtx.unlock();

				if (doDelete)
				{
					delete this;
				}
			}

			void incRef(unsigned int n = 1)
			{
				m_mtx.lock();
				m_refCount += n;
				m_mtx.unlock();
			}

			void setDone(bool yes)
			{
				m_mtx.lock();
				m_isDone = yes;
				m_cond.notify_all();
				m_mtx.unlock();
			}
		private:
			boost::mutex m_mtx;
			boost::condition m_cond;
			bool m_isDone;
			unsigned int m_refCount;
		};

		class _impl_Event
		{
		public:
			_impl_Event()
				: m_blockHandle(NULL)
			{
			}
			~_impl_Event()
			{
			}

			void setBlockHandleDone()
			{
				if (m_blockHandle != NULL)
				{
					m_blockHandle->setDone(true);
				}
			}

			void setBlockHandle(BlockHandle *h)
			{
				m_blockHandle = h;
			}

			BlockHandle * blockHandle() const
			{
				return m_blockHandle;
			}
		private:
			BlockHandle * m_blockHandle;
		};

		class _impl_Queue
		{
		public:
			_impl_Queue()
				: m_doKeepRunning(true)
			{
			}

			void postEvent(Event *e)
			{
				m_mtx.lock();
				if (m_doKeepRunning)
				{
					m_events.push(e);
					m_cond.notify_all();
				}
				m_mtx.unlock();
			}

			BlockHandle * postEventWithBlockHandle(Event *e)
			{
				BlockHandle *h = new BlockHandle;

				m_mtx.lock();
				if (m_doKeepRunning)
				{
					e->pimpl()->setBlockHandle(h);
					m_events.push(e);
					m_cond.notify_all();
				}
				else
				{
					// block handle is functionally pointless, but we return one anyway
					h->setDone(true);
				}
				m_mtx.unlock();

				return h;
			}

			bool doKeepRunning()
			{
				m_mtx.lock();
				bool r = m_doKeepRunning;
				m_mtx.unlock();
				return r;
			}

			void setDoKeepRunning(bool yes)
			{
				m_mtx.lock();
				m_doKeepRunning = yes;
				m_mtx.unlock();
			}

			void run(void *data)
			{
				Event *e;

				while (doKeepRunning())
				{
					e = pullEvent();
					e->run(data);
					e->pimpl()->setBlockHandleDone();
					delete e;
				}
				// not running anymore
				// dispose all events
				m_mtx.lock();
				while (!m_events.empty())
				{
					e = m_events.front();
					m_events.pop();
					e->pimpl()->setBlockHandleDone();
					delete e;
				}
				m_mtx.unlock();
			}
		private:
			std::queue<Event*> m_events;
			boost::mutex m_mtx;
			boost::condition m_cond;
			bool m_doKeepRunning;

			Event * pullEvent()
			{
				m_mtx.lock();

				while (m_events.empty())
				{
					m_cond.wait(m_mtx);
				}
				Event *e = m_events.front();
				m_events.pop();
				m_mtx.unlock();
				return e;
			}
		};

		void incBlockHandleRef(BlockHandle *h)
		{
			h->incRef();
		}

		void blockOnHandle(BlockHandle *h)
		{
			h->block();
		}

		Event::Event()
		{
			m_pimpl = new _impl_Event;
		}

		Event::~Event()
		{
			delete m_pimpl;
		}

		_impl_Event * Event::pimpl() const
		{
			return m_pimpl;
		}

		Queue::Queue()
		{
			m_pimpl = new _impl_Queue;
		}

		Queue::~Queue()
		{
			delete m_pimpl;
		}

		void Queue::postEvent(Event *e)
		{
			m_pimpl->postEvent(e);
		}

		BlockHandle * Queue::postEventWithBlockHandle(Event *e)
		{
			return m_pimpl->postEventWithBlockHandle(e);
		}

		bool Queue::doKeepRunning()
		{
			return m_pimpl->doKeepRunning();
		}

		void Queue::setDoKeepRunning(bool yes)
		{
			m_pimpl->setDoKeepRunning(yes);
		}

		void Queue::run(void *data)
		{
			m_pimpl->run(data);
		}
	}
}
