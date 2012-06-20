#ifndef CORE_RINGBUFFER_HPP
#define CORE_RINGBUFFER_HPP

#include "types.hpp"
#include <string.h>

namespace core
{
	// Please do NOT use non-plain-old-data structures as T!
	template<typename T, int Size>
	class RingBuffer
	{
	public:
		RingBuffer()
			: m_readpos(0), m_writepos(0), m_tendencytofull(false)
		{
		}
		~RingBuffer()
		{
		}

		Quantity availRead() const
		{
			if (m_readpos == m_writepos)
			{
				return m_tendencytofull ? Size : 0;
			}
			if (m_writepos < m_readpos)
				return Size+m_writepos - m_readpos;

			return m_writepos - m_readpos;
		}
		Quantity availWrite() const
		{
			return Size - availRead();
		}
		bool isFull() const
		{
			return availWrite() == 0;
		}
		bool isEmpty() const
		{
			return availRead() == 0;
		}
		Quantity read(T *data, Quantity sz)
		{
			// clip sz to availRead()
			Quantity avail = availRead();
			if (sz > avail)
				sz = avail;

			if (sz == 0)
				return 0;

			bool wraps = m_readpos+sz > Size;

			if (wraps)
			{
				Quantity r = Size-m_readpos;
				memcpy(data, m_buffer+m_readpos, r*sizeof(T));
				memcpy(data+r, m_buffer, (sz-r)*sizeof(T));
			}
			else
			{
				memcpy(data, m_buffer+m_readpos, sz*sizeof(T));
			}
			_advanceRead(sz);

			return sz;
		}
		Quantity skipRead(Quantity sz)
		{
			// clip sz to availRead()
			Quantity avail = availRead();
			if (sz > avail)
				sz = avail;

			_advanceRead(sz);

			return sz;
		}
		Quantity write(const T *data, Quantity sz)
		{
			// clip sz to availWrite()
			Quantity avail = availWrite();
			if (sz > avail)
				sz = avail;

			if (sz == 0)
				return 0;

			bool wraps = m_writepos+sz > Size;

			if (wraps)
			{
				Quantity r = Size-m_writepos;
				memcpy(m_buffer+m_writepos, data, r*sizeof(T));
				memcpy(m_buffer, data+r, (sz-r)*sizeof(T));
			}
			else
			{
				memcpy(m_buffer+m_writepos, data, sz*sizeof(T));
			}
			_advanceWrite(sz);

			return sz;
		}
		void clear()
		{
			m_readpos = m_writepos = 0;
			m_tendencytofull = false;
		}

	private:
		void _advanceRead(Quantity a)
		{
			m_readpos = (m_readpos + a) % Size;
			m_tendencytofull = false;
		}
		void _advanceWrite(Quantity a)
		{
			m_writepos = (m_writepos + a) % Size;
			m_tendencytofull = true;
		}

		Quantity m_readpos, m_writepos;
		bool m_tendencytofull;
		T m_buffer[Size];
	};
}

#endif

