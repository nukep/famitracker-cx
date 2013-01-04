#ifndef CORE_THREADPOOL_HPP
#define CORE_THREADPOOL_HPP

namespace core
{
	namespace threadpool
	{
		class _impl_Event;
		class _impl_Queue;

		class BlockHandle;

		// If multiple threads are blocking on a handle, multiple block calls
		// have to be run before deleting the handle. This tells the handle
		// to expect an additional blockOnHandle() call.
		// Must be called before the first blockOnHandle() call.
		void incBlockHandleRef(BlockHandle *h);

		// Blocks the thread until the event associated with the handle is finished
		void blockOnHandle(BlockHandle *h);

		class Event
		{
		public:
			Event();
			virtual ~Event();
			virtual void run(void *data) const = 0;

			_impl_Event * pimpl() const;
		private:
			_impl_Event * m_pimpl;
		};

		class Queue
		{
		public:
			Queue();
			~Queue();

			void postEvent(Event *e);

			// When using postEventWithBlockHandle, the block handle should
			// be used at least once (with blockOnHandle()).
			// Not doing so will result in a memory leak
			BlockHandle * postEventWithBlockHandle(Event *e);

			bool doKeepRunning();
			void setDoKeepRunning(bool yes);

			void run(void *data);
		private:
			_impl_Queue * m_pimpl;
		};
	}
}

#endif

