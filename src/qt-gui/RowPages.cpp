#include "RowPages.hpp"
#include "famitracker-core/FtmDocument.hpp"
#include <stdio.h>
#include <QPainter>

namespace gui
{
	const int pixmap_rows = 1<<4;

	RowPages::RowPages()
	{
		m_requestCallback = NULL;
	}
	RowPages::~RowPages()
	{
		clear();
	}

	void RowPages::setRequestCallback(requestCallback_f f)
	{
		m_requestCallback = f;
	}

	void RowPages::clear()
	{
		RowPagesList::iterator it;
		for (it = m_rowPages.begin(); it != m_rowPages.end(); ++it)
		{
			rowpage_t &r = *it;
			r.clean();
		}
		m_rowPages.clear();
	}

	void RowPages::cullPages(unsigned int frame,
							 unsigned int min_frame, unsigned int min_row_index,
							 unsigned int max_frame, unsigned int max_row_index)
	{
		RowPagesList::iterator it;
		for (it = m_rowPages.begin(); it != m_rowPages.end(); )
		{
			rowpage_t &r = *it;
			bool onSelected = (r.frame == frame);
			bool keepPage;
			keepPage = r.selected == onSelected && r.isWithin(min_frame, min_row_index, max_frame, max_row_index);

			if (keepPage)
			{
				++it;
			}
			else
			{
				r.clean();
				it = m_rowPages.erase(it);
			}
		}
	}

	struct addRowPage_t
	{
		RowPages::requestCallback_f requestCallback;
		RowPagesList *rowPages;
		void *data;
		bool cull;
		unsigned int min_frame, min_row_index,
					 max_frame, max_row_index;
	};

	static void addRowPage(const addRowPage_t &t, rowpage_t rp)
	{
		bool request = true;
		if (t.cull)
		{
			if (rp.isWithin(t.min_frame, t.min_row_index, t.max_frame, t.max_row_index))
			{
				request = false;
			}
		}
		if (request)
		{
		//	fprintf(stderr, "request: frame %2d, rowindex %2d, length %2d\n", rp.frame, rp.row_index, rp.row_count);
			(*t.requestCallback)(&rp, pixmap_rows, t.data);
			t.rowPages->push_back(rp);
		}
	}

	void RowPages::requestRowPages(const FtmDocument *d, void *data, int from, int to, unsigned int frame)
	{
		// min_row_index is the first visible row page index of min
		// ***_frame_row_remainder is
		// ***_row_end_index is the  last logical row page index of ***
		// ***_row_max_index is the  last visible row page index of ***

		unsigned int min_frame, min_row_index;
		unsigned int min_frame_row_remainder;
		unsigned int min_row_max_index;

		unsigned int cur_frame_row_remainder;
		unsigned int cur_row_end_index;
		unsigned int cur_row_max_index;

		unsigned int max_frame;
		unsigned int max_frame_row_remainder;
		unsigned int max_row_end_index;
		unsigned int max_row_max_index;

		bool request_frame_before = false;
		bool request_frame_after = false;

		unsigned int min_frame_pl;
		unsigned int cur_frame_pl = d->getFramePlayLength(frame);
		unsigned int max_frame_pl;
		cur_frame_row_remainder = cur_frame_pl % pixmap_rows;
		cur_row_end_index = (cur_frame_pl-1) / pixmap_rows;

		if (from < 0 && frame != 0)
		{
			request_frame_before = true;
		}
		if (request_frame_before)
		{
			// we need to render frame-1
			min_frame = frame-1;
			min_frame_pl = d->getFramePlayLength(min_frame);
			int min_frame_from;
			if (-from >= min_frame_pl)
			{
				// shows more than the entire previous frame
				min_frame_from = 0;
			}
			else
			{
				// shows part of the previous frame
				min_frame_from = from + (int)min_frame_pl;
			}
			min_row_index = min_frame_from / pixmap_rows;
			min_frame_row_remainder = min_frame_pl % pixmap_rows;
			min_row_max_index = (min_frame_pl-1) / pixmap_rows;
		}
		else
		{
			min_frame = frame;
			min_frame_pl = cur_frame_pl;
			if (from >= 0)
				min_row_index = from / pixmap_rows;
			else
				min_row_index = 0;
			min_frame_row_remainder = cur_frame_row_remainder;
			min_row_max_index = cur_row_end_index;
		}
		int max_frame_to;
		if (to >= (int)cur_frame_pl)
		{
			unsigned int doc_max_frame = d->GetFrameCount()-1;
			if (frame+1 <= doc_max_frame)
			{
				// not the last frame
				request_frame_after = true;
			}
		}
		if (request_frame_after)
		{
			// we need to render frame+1
			max_frame = frame+1;
			max_frame_to = to - (int)cur_frame_pl;
			max_frame_pl = d->getFramePlayLength(max_frame);
			max_frame_row_remainder = max_frame_pl % pixmap_rows;
			max_row_end_index = (max_frame_pl-1) / pixmap_rows;
		}
		else
		{
			max_frame = frame;
			max_frame_to = to;
			max_frame_pl = cur_frame_pl;
			max_frame_row_remainder = cur_frame_row_remainder;
			max_row_end_index = cur_row_end_index;
		}

		if (max_frame_to >= max_frame_pl)
		{
			// shows more than the entire next frame
			max_frame_to = max_frame_pl-1;
		}
		if (request_frame_after)
		{
			cur_row_max_index = cur_row_end_index;
		}
		else
		{
			cur_row_max_index = max_frame_to / pixmap_rows;
		}
		max_row_max_index = max_frame_to / pixmap_rows;

		// do some cleaning
		cullPages(frame, min_frame, min_row_index, max_frame, max_row_max_index);

		// request the pages

		RowPagesList oldlist;
		oldlist.swap(m_rowPages);

		addRowPage_t arp;
		arp.data = data;
		arp.requestCallback = m_requestCallback;
		arp.rowPages = &m_rowPages;
		arp.cull = !oldlist.empty();

		if (arp.cull)
		{
			const rowpage_t &f = oldlist.front();
			const rowpage_t &t = oldlist.back();
			arp.min_frame = f.frame;
			arp.min_row_index = f.row_index;
			arp.max_frame = t.frame;
			arp.max_row_index = t.row_index;
		}

		rowpage_t rp;
		unsigned int cur_begin;
		unsigned int i;
		if (request_frame_before)
		{
			rp.frame = min_frame;
			rp.selected = false;
			rp.row_count = pixmap_rows;
			for (i = min_row_index; i < min_row_max_index; i++)
			{
				rp.row_index = i;
				addRowPage(arp, rp);
			}
			rp.row_index = i;
			// the last row page of the previous frame is always rendered here
			if (min_frame_row_remainder != 0)
			{
				rp.row_count = min_frame_row_remainder;
			}

			addRowPage(arp, rp);
			cur_begin = 0;
		}
		else
		{
			cur_begin = min_row_index;
		}
		if (true)	// always request the current frame's rows
		{
			rp.frame = frame;
			rp.selected = true;
			rp.row_count = pixmap_rows;
			for (i = cur_begin; i < cur_row_max_index; i++)
			{
				rp.row_index = i;
				addRowPage(arp, rp);
			}
			rp.row_index = i;
			if (i == cur_row_end_index && cur_frame_row_remainder != 0)
			{
				rp.row_count = cur_frame_row_remainder;
			}

			addRowPage(arp, rp);
		}
		if (request_frame_after)
		{
			rp.frame = max_frame;
			rp.selected = false;
			rp.row_count = pixmap_rows;
			for (i = 0; i < max_row_max_index; i++)
			{
				rp.row_index = i;
				addRowPage(arp, rp);
			}
			rp.row_index = i;
			if (i == max_row_end_index && max_frame_row_remainder != 0)
			{
				rp.row_count = max_frame_row_remainder;
			}

			addRowPage(arp, rp);
		}
		if (!oldlist.empty())
		{
			const rowpage_t &begin = *oldlist.begin();
			// insert the old list into the new one
			RowPagesList::iterator it;
			for (it = m_rowPages.begin(); it != m_rowPages.end(); ++it)
			{
				const rowpage_t &c = *it;
				// break out as soon as 'c' is after 'begin'
				if (c.frame > begin.frame)
				{
					break;
				}
				else if (c.frame == begin.frame)
				{
					if (c.row_index > begin.row_index)
					{
						break;
					}
				}
			}
			m_rowPages.insert(it, oldlist.begin(), oldlist.end());
			// 'it' is after the oldlist elements
		}
	}

	void RowPages::render(QPainter &p, unsigned int frame, int from, int row_height)
	{
		int y = 0;
		if (from > 0)
		{
			y = (from / pixmap_rows)*pixmap_rows*row_height;
		}

		// offset 'y' by how many pixels is before the start of the current frame
		for (RowPagesList::const_iterator it = m_rowPages.begin(); it != m_rowPages.end(); ++it)
		{
			const rowpage_t &rp = *it;
			if (rp.frame == frame)
				break;
			y -= rp.image->height();
		}

		for (RowPagesList::const_iterator it = m_rowPages.begin(); it != m_rowPages.end(); ++it)
		{
			const rowpage_t &rp = *it;
			p.drawImage(0, y, *rp.image);
			y += rp.image->height();
		}
	}
}
