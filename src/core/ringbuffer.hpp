#ifndef CORE_RINGBUFFER_HPP
#define CORE_RINGBUFFER_HPP

#include "types.hpp"
#include <string.h>

namespace core
{
	// Please do NOT use non-plain-old-data structures as T!
	class RingBuffer
	{
	public:
		RingBuffer(Quantity elementSize=1)
			: m_elementcount(0), m_readpos(0), m_writepos(0), m_tendencytofull(false),
			  m_elementsize(elementSize)
		{
			m_buffer = new core::byte[m_elementcount*m_elementsize];
		}
		~RingBuffer()
		{
			delete[] m_buffer;
		}

		void resize(Quantity total_elements)
		{
			m_elementcount = total_elements;
			core::byte *buf = new core::byte[total_elements*m_elementsize];
			clear();
			delete[] m_buffer;
			m_buffer = buf;
		}
		Quantity availRead() const
		{
			if (m_readpos == m_writepos)
			{
				return m_tendencytofull ? m_elementcount : 0;
			}
			if (m_writepos < m_readpos)
				return m_elementcount+m_writepos - m_readpos;

			return m_writepos - m_readpos;
		}
		Quantity availWrite() const
		{
			return m_elementcount - availRead();
		}
		bool isFull() const
		{
			return availWrite() == 0;
		}
		bool isEmpty() const
		{
			return availRead() == 0;
		}
		Quantity read(void *data, Quantity sz)
		{
			// clip sz to availRead()
			Quantity avail = availRead();
			if (sz > avail)
				sz = avail;

			if (sz == 0)
				return 0;

			bool wraps = m_readpos+sz > m_elementcount;

			if (wraps)
			{
				Quantity r = m_elementcount-m_readpos;
				memcpy(data, m_buffer+m_readpos*m_elementsize, r*m_elementsize);
				memcpy((core::byte*)data+(r*m_elementsize), m_buffer, (sz-r)*m_elementsize);
			}
			else
			{
				memcpy(data, m_buffer+m_readpos*m_elementsize, sz*m_elementsize);
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
		Quantity write(const void *data, Quantity sz)
		{
			// clip sz to availWrite()
			Quantity avail = availWrite();
			if (sz > avail)
				sz = avail;

			if (sz == 0)
				return 0;

			bool wraps = m_writepos+sz > m_elementcount;

			if (wraps)
			{
				Quantity r = m_elementcount-m_writepos;
				memcpy(m_buffer+m_writepos*m_elementsize, data, r*m_elementsize);
				memcpy(m_buffer, (core::byte*)data+(r*m_elementsize), (sz-r)*m_elementsize);
			}
			else
			{
				memcpy(m_buffer+m_writepos*m_elementsize, data, sz*m_elementsize);
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
			m_readpos = (m_readpos + a) % m_elementcount;
			if (m_readpos == m_writepos)
			{
				// ringbuffer is empty, reset to position 0. reduces wrapping
				m_readpos = m_writepos = 0;
			}
			m_tendencytofull = false;
		}
		void _advanceWrite(Quantity a)
		{
			m_writepos = (m_writepos + a) % m_elementcount;
			m_tendencytofull = true;
		}
		bool _readWraps() const
		{
			return m_readpos + availRead() > m_elementcount;
		}
		Quantity m_elementcount;
		Quantity m_elementsize;
		Quantity m_readpos, m_writepos;
		bool m_tendencytofull;
		core::byte *m_buffer;
	};
}

#endif

