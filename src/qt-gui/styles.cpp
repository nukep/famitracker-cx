#include <string.h>
#include "styles.hpp"

namespace styles
{
	static color_t color_default(Colors c)
	{
		switch (c)
		{
		case PATTERN_BG:
			return color_t(0,0,0);
		case PATTERN_HIGHLIGHT1_BG:
			return color_t(16,16,0);
		case PATTERN_HIGHLIGHT2_BG:
			return color_t(32,32,0);
		case PATTERN_FG:
			return color_t(0,255,0);
		case PATTERN_HIGHLIGHT1_FG:
			return color_t(240,240,0);
		case PATTERN_HIGHLIGHT2_FG:
			return color_t(255,255,96);
		case PATTERN_INST:
			return color_t(128,255,128);
		case PATTERN_VOL:
			return color_t(128,128,255);
		case PATTERN_EFFNUM:
			return color_t(255,128,128);
		case PATTERN_SELECTION:
			return color_t(96,0,255);
		case PATTERN_CURSOR:
			return color_t(128,128,128);
		}
	}
	static color_t color_monochrome(Colors c)
	{
		switch (c)
		{
		case PATTERN_BG:
			return color_t(24,24,24);
		case PATTERN_HIGHLIGHT1_BG:
			return color_t(32,32,32);
		case PATTERN_HIGHLIGHT2_BG:
			return color_t(48,48,48);
		case PATTERN_FG:
			return color_t(192,192,192);
		case PATTERN_HIGHLIGHT1_FG:
			return color_t(240,240,240);
		case PATTERN_HIGHLIGHT2_FG:
			return color_t(255,255,255);
		case PATTERN_INST:
			return color_t(128,255,128);
		case PATTERN_VOL:
			return color_t(128,128,255);
		case PATTERN_EFFNUM:
			return color_t(255,128,128);
		case PATTERN_SELECTION:
			return color_t(80,69,69);
		case PATTERN_CURSOR:
			return color_t(128,128,144);
		}
	}

	static int style = 0;

	color_t color(Colors c)
	{
		if (style == 0)
			return color_default(c);
		else if (style == 1)
			return color_monochrome(c);

		return color_default(c);
	}

	void selectStyle(const char *name)
	{
		if (strcmp(name, "Default") == 0)
		{
			style = 0;
		}
		else if (strcmp(name, "Monochrome") == 0)
		{
			style = 1;
		}
	}
}
