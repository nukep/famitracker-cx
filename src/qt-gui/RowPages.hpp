#ifndef _ROWPAGES_HPP_
#define _ROWPAGES_HPP_

#include <list>
#include <QImage>

class FtmDocument;

namespace gui
{
	/*
	Provide row pages to the utilizing display
	Responsible for managing and requesting rendered pixmaps
	NOT responsible for solving geometry problems
	A row page includes a pixmap of a fixed amount of rows
	*/

	struct rowpage_t
	{
		QImage *image;
		unsigned int frame;
		unsigned int row_index;		// row_index * pixmap_rows = starting row
		unsigned int row_count;
		bool selected;

		bool isWithin(unsigned int min_frame, unsigned int min_row_index,
					  unsigned int max_frame, unsigned int max_row_index)
		{
			return
			((frame >= min_frame) && (frame != min_frame || row_index >= min_row_index))
			&&
			((frame <= max_frame) && (frame != max_frame || row_index <= max_row_index));
		}

		void clean()
		{
			delete image;
		}
	};
	typedef std::list<rowpage_t> RowPagesList;
	class RowPages
	{
	public:
		typedef void (*requestCallback_f)(rowpage_t *r, unsigned int rowpagesize, void *data);
		RowPages();
		~RowPages();
		void setRequestCallback(requestCallback_f f);
		void clear();
		void requestRowPages(const FtmDocument *d, void *data, int from, int to, unsigned int frame);
		void render(QPainter &p, unsigned int frame, int from, int row_height);
	private:
		// clear invalidated pages (out of range or different selection)
		void cullPages(unsigned int frame,
					   unsigned int min_frame, unsigned int min_row_index,
					   unsigned int max_frame, unsigned int max_row_index);

		requestCallback_f m_requestCallback;
		// list guaranteed to (MUST) be in order
		RowPagesList m_rowPages;
	};
}

#endif

