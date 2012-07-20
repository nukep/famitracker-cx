#ifndef _STYLES_HPP_
#define _STYLES_HPP_

namespace styles
{
	enum Colors
	{
		PATTERN_BG,
		PATTERN_HIGHLIGHT1_BG,
		PATTERN_HIGHLIGHT2_BG,

		PATTERN_FG,
		PATTERN_HIGHLIGHT1_FG,
		PATTERN_HIGHLIGHT2_FG,

		PATTERN_INST,
		PATTERN_VOL,
		PATTERN_EFFNUM,

		PATTERN_SELECTION,
		PATTERN_CURSOR
	};

	struct color_t
	{
		typedef unsigned char T;
		color_t(T r, T g, T b) : r(r), g(g), b(b){}
		T r, g, b;
	};

	void selectStyle(const char *name);

	color_t color(Colors c);
}

#endif

