#include <ncurses.h>
#include <signal.h>
#include <string.h>
#include "famitracker-core/App.hpp"
#include "famitracker-core/Document.hpp"
#include "famitracker-core/FtmDocument.hpp"
#include "famitracker-core/sound.hpp"
#include "famitracker-core/TrackerController.hpp"
#include "../parse_arguments.hpp"
#include "../defaults.hpp"

class Session
{
public:
	enum
	{
		S_NONE=0, S_CURRENTROW,
		S_COUNT
	};
	enum
	{
		C_BLANK=0, C_ROW, C_NOTE, C_INST, C_VOL, C_EFFNUM, C_EFFPARAM,
		C_COUNT
	};
	enum
	{
		SC_CHANNELS=0,
		SC_COUNT
	};
	FtmDocument *doc;
	SoundGen *sgen;
	int m_lastFrame;
	int m_lastRowTop;
	int m_lastRowCurrent;
	int m_frame;
	int m_rowTop;
	int m_rowCurrent;
	int m_width;
	int m_height;
	int m_patwidth;
	int m_patheight;

	bool m_updateScreen;
	bool m_updatePattern;

	Session()
	{
		m_colorPairCount = 0;
		m_lastFrame = 0;
		m_lastRowTop = 0;
		m_lastRowCurrent = 0;
		m_frame = 0;
		m_rowTop = 0;
		m_rowCurrent = 0;
		m_width = 0;
		m_height = 0;
		m_patwidth = 0;
		m_patheight = 0;
	}

	void setWidthHeight(int w, int h)
	{
		if (m_width != w || m_height != h)
		{
			m_updateScreen = true;
		}
		m_width = w;
		m_height = h;
		m_patwidth = w;
		m_patheight = h-1;
	}

	void setRowFrame(int row, int frame)
	{
		m_lastFrame = m_frame;
		m_lastRowTop = m_rowTop;
		m_lastRowCurrent = m_rowCurrent;

		if (frame != m_frame)
		{
			m_frame = frame;
			m_rowTop = 0;
			m_updateScreen = true;
			m_updatePattern = true;
		}

		if (m_rowCurrent != row)
		{
			m_updatePattern = true;
		}

		m_rowCurrent = row;

		int rowTo = m_rowTop + m_patheight - 1;
		if (row > rowTo)
		{
			m_rowTop = row;
			m_updateScreen = true;
		}
		if (row < m_rowTop)
		{
			m_rowTop = row;
			m_updateScreen = true;
		}
	}

	void resetUpdateFlags()
	{
		m_updateScreen = false;
		m_updatePattern = false;
	}

	void setColorPair(WINDOW *w, int scheme, int c) const
	{
		wcolor_set(w, m_colorPairs[scheme][c], NULL);
	}

	void setSpecialColorPair(WINDOW *w, int sc) const
	{
		wcolor_set(w, m_specialColorPairs[sc], NULL);
	}

	void initColorPair(int scheme, int bgc)
	{
		addColorPair(scheme, C_BLANK, COLOR_CYAN, bgc);
		addColorPair(scheme, C_ROW, COLOR_GREEN, bgc);
		addColorPair(scheme, C_NOTE, COLOR_GREEN, bgc);
		addColorPair(scheme, C_INST, COLOR_YELLOW, bgc);
		addColorPair(scheme, C_VOL, COLOR_BLUE, bgc);
		addColorPair(scheme, C_EFFNUM, COLOR_MAGENTA, bgc);
		addColorPair(scheme, C_EFFPARAM, COLOR_GREEN, bgc);
	}

	void initColorPairs()
	{
		m_colorPairCount = 0;
		addSpecialColorPair(SC_CHANNELS, COLOR_WHITE, COLOR_BLACK);
		initColorPair(S_NONE, COLOR_BLACK);
		initColorPair(S_CURRENTROW, COLOR_BLUE);
	}
private:
	void addColorPair(int scheme, int c, int fgc, int bgc)
	{
		m_colorPairs[scheme][c] = ++m_colorPairCount;
		init_pair(m_colorPairCount, fgc, bgc);
	}
	void addSpecialColorPair(int sc, int fgc, int bgc)
	{
		m_specialColorPairs[sc] = ++m_colorPairCount;
		init_pair(m_colorPairCount, fgc, bgc);
	}

	int m_colorPairCount;
	int m_colorPairs[S_COUNT][C_COUNT];
	int m_specialColorPairs[SC_COUNT];
};

const char *default_sound=DEFAULT_SOUND;
static Session *tracker_session;

struct arguments_t
{
	bool help;

	int track;
	int sampleRate;
	std::string sound;
	std::string file;
};

static void parse_arguments(int argc, char *argv[], arguments_t &a)
{
	ParseArguments pa;
	const char *flagfields[] = {"-help"};
	pa.setFlagFields(flagfields, 1);
	pa.parse(argv, argc);

	a.help = pa.flag("-help");

	if (a.help)
		return;

	a.track = pa.integer("t", 1);
	a.sampleRate = pa.integer("sr", 48000);
	a.sound = pa.string("sound", default_sound);
	a.file = pa.string(0);
}

static inline int channelWidth(int effColumns)
{
	return 3 + 1+2 + 1+1 + 4*(effColumns);
}

static void noteNotation(unsigned int note, char *out)
{
	static const char notesharp_letters[] = "CCDDEFFGGAAB";
	static const char notesharp_sharps[] =  " # #  # # # ";
	static const char noteflat_letters[] =  "CDDEEFGGAABB";
	static const char noteflat_flats[] =    " b b  b b b ";

	if (note >= 12)
	{
		// noticeably errors
		out[0] = '!';
		out[1] = '!';
		return;
	}

	bool m_notesharps = true;

	if (m_notesharps)
	{
		out[0] = notesharp_letters[note];
		out[1] = notesharp_sharps[note];
	}
	else
	{
		out[0] = noteflat_letters[note];
		out[1] = noteflat_flats[note];
	}
}

static void replaceCharWith(char *buf, char from, char to)
{
	while (*buf != 0)
	{
		if (*buf == from)
		{
			*buf = to;
		}
		buf++;
	}
}

static void printBlank(WINDOW *w, Session &s, int scheme, int spaces, bool leading=true)
{
	wattroff(w, A_BOLD);
	s.setColorPair(w, scheme, Session::C_BLANK);
	if (leading)
	{
		wprintw(w, " ");
	}
	for (int i = 0; i < spaces; i++)
	{
		waddch(w, '-');
	}
	wattron(w, A_BOLD);
}

void paintNote(WINDOW *w, Session &s, const stChanNote &note, int channel, int effColumns, int scheme)
{
	wattron(w, A_BOLD);
	char buf[5];

	// todo: use enumerator constant
	bool noiseChannel = channel == 3;

	// Note
	s.setColorPair(w, scheme, Session::C_NOTE);
	if (note.Note == NONE)
	{
		printBlank(w, s, scheme, 3, false);
	}
	else if (note.Note == RELEASE)
	{
		// two bars
		for (int i = 0; i < 3; i++)
			waddch(w, '=' | A_BOLD);
	}
	else if (note.Note == HALT)
	{
		// one bar
		for (int i = 0; i < 3; i++)
			waddch(w, ACS_HLINE | A_BOLD);
	}
	else
	{
		int ni = note.Note - C;
		buf[2] = 0;
		if (!noiseChannel)
		{
			noteNotation(ni, buf);
			replaceCharWith(buf, ' ', '-');
			wprintw(w, "%s%d", buf, note.Octave);
		}
		else
		{
			ni = (ni + note.Octave*12) % 16;
			wprintw(w, "%X-#", ni);
		}
	}

	// Instrument
	s.setColorPair(w, scheme, Session::C_INST);
	if (note.Instrument >= MAX_INSTRUMENTS)
	{
		printBlank(w, s, scheme, 2);
	}
	else
	{
		wprintw(w, " %02X", note.Instrument);
	}

	// Volume
	s.setColorPair(w, scheme, Session::C_VOL);
	if (note.Vol > 0xF)
	{
		printBlank(w, s, scheme, 1);
	}
	else
	{
		wprintw(w, " %01X", note.Vol);
	}

	// EFF

	bool terminate;

	buf[3] = 0;

	for (int i = 0; i <= effColumns; i++)
	{
		unsigned char eff = note.EffNumber[i];

		if (eff == EF_JUMP || eff == EF_SKIP || eff == EF_HALT)
			terminate = true;

		if (eff == 0)
		{
			printBlank(w, s, scheme, 3);
		}
		else
		{
			buf[0] = EFF_CHAR[eff-1];

			s.setColorPair(w, scheme, Session::C_EFFNUM);
			wprintw(w, " %c", buf[0]);

			if (eff == 0)
			{
				printBlank(w, s, scheme, 2, false);
			}
			else
			{
				s.setColorPair(w, scheme, Session::C_EFFPARAM);
				unsigned char effp = note.EffParam[i];
				wprintw(w, "%02X", effp);
			}
		}
	}
	wattroff(w, A_BOLD);
}

static void paintrow(WINDOW *w, Session &s, int channels, int frame, int row, int scheme)
{
	s.setColorPair(w, Session::S_NONE, Session::C_ROW);
	wprintw(w, "%02X", row);
	waddch(w, ACS_VLINE);
	int leftoverWidth = s.m_patwidth;
	leftoverWidth -= 2
			+ 1		// extra |
			+ 3		// "more channels" indicator
			;

	for (int chan = 0; chan < channels; chan++)
	{
		if (chan != 0)
		{
			s.setColorPair(w, scheme, Session::C_ROW);
			waddch(w, ACS_VLINE);
		}
		int effColumns = s.doc->GetEffColumns(chan);
		int width = channelWidth(effColumns+1)+1;
		leftoverWidth -= width;
		if (leftoverWidth < 0)
		{
			break;
		}
		stChanNote note;
		s.doc->GetNoteData(frame, chan, row, &note);
		paintNote(w, s, note, chan, effColumns, scheme);
	}
	if (leftoverWidth < 0)
	{
		// the "more channels" indicator
		wprintw(w, ">>");
	}
	wprintw(w, "\n");
}

static void paintChannelNames(WINDOW *w, Session &s)
{
	int channels = s.doc->GetAvailableChannels();

	s.setSpecialColorPair(w, Session::SC_CHANNELS);

	int leftoverWidth = s.m_patwidth;
	leftoverWidth -= 2
			+ 1		// extra |
			+ 3		// "more channels" indicator
			;
	int nx = 3;
	char buf[64];
	for (int i = 0; i < channels; i++)
	{
		int effColumns = s.doc->GetEffColumns(i);
		int width = channelWidth(effColumns+1)+1;
		leftoverWidth -= width;
		if (leftoverWidth < 0)
		{
			break;
		}

		wmove(w, 0, nx);
		int colw = width - 1;
		int sz = sizeof(buf)-1;
		if (colw < sz)
		{
			sz = colw;
		}
		snprintf(buf, sz+1, "%s", app::channelMap()->GetChannelName(s.doc->getChannelsFromChip()[i]));
		wprintw(w, "%s", buf);
		nx += colw+1;
	}

	wprintw(w, "\n");
}

static void paintpattern(WINDOW *w, Session &s)
{
	int x, y;
	getmaxyx(w, y, x);

	int frame = s.m_frame;
	int channels = s.doc->GetAvailableChannels();

	int currentRow = s.m_rowCurrent;
	int topRow = s.m_rowTop;

	paintChannelNames(w, s);
	y--;

	int rowCount = s.doc->getFramePlayLength(frame);
	if (rowCount > y+s.m_rowTop)
		rowCount = y+s.m_rowTop;

	for (int row = topRow; row < rowCount; row++)
	{
		int scheme = row==currentRow ? Session::S_CURRENTROW : Session::S_NONE;
		paintrow(w, s, channels, frame, row, scheme);
	}
}

void drawwindow(Session &s)
{
	start_color();
	clear();

	s.initColorPairs();

	paintpattern(stdscr, s);
	move(0,0);
	refresh();
}

void updatePattern(Session &s)
{
	WINDOW *w = stdscr;

	int frame = s.m_frame;
	int channels = s.doc->GetAvailableChannels();

	// y, x
	move(s.m_lastRowCurrent - s.m_lastRowTop + 1, 0);
	paintrow(w, s, channels, s.m_lastFrame, s.m_lastRowCurrent, Session::S_NONE);

	move(s.m_rowCurrent - s.m_rowTop + 1, 0);
	paintrow(w, s, channels, s.m_frame, s.m_rowCurrent, Session::S_CURRENTROW);

	move(0,0);
	refresh();
}

void update(Session &s)
{
	if (s.m_updateScreen)
	{
		// redraw the screen
		endwin();
		initscr();
		drawwindow(s);
	}
	else if (s.m_updatePattern)
	{
		// just update the patterns
		// this is simply reprinting the last and current rows
		updatePattern(s);
	}
	s.resetUpdateFlags();
}

void handle_winch(int sig)
{
	signal(SIGWINCH, SIG_IGN);

	// Reinitialize the window to update data structures.
	endwin();
	initscr();
	drawwindow(*tracker_session);

	signal(SIGWINCH, handle_winch);
}

static void tracker_update(SoundGen::rowframe_t rf, FtmDocument *doc, void *data)
{
	if (!rf.rowframe_changed)
		return;

	Session &s = *((Session*)data);

	int y, x;
	getmaxyx(stdscr, y, x);

	s.setWidthHeight(x, y);
	s.setRowFrame(rf.row, rf.frame);

	update(s);
}

int runSong(Session &s, const char *sound)
{
	unsigned int rate = 48000;

	core::SoundSinkPlayback *sink = (core::SoundSinkPlayback*)core::loadSoundSink(sound);
	if (sink == NULL)
	{
		return 1;
	}
	sink->initialize(rate, 1, 150);

	SoundGen *sg = new SoundGen;
	s.sgen = sg;
	sg->setSoundSink(sink);
	sg->setDocument(s.doc);
	sg->setTrackerUpdate(tracker_update, &s);

	sg->trackerController()->startAt(0, 0);
	sg->startTracker();
	sink->blockUntilStopped();
	sink->blockUntilTimerEmpty();

	delete sink;
	delete sg;
}

int main(int argc, char *argv[])
{
	const char *sound;
	const char *song;
	int track;

	arguments_t args;
	parse_arguments(argc-1, argv+1, args);

	if (args.help)
	{
	//	print_help();
		return 0;
	}

	if (args.file.empty())
	{
		printf("Please specify a song\n\n");
	//	print_help();
		return 1;
	}
	else
	{
		song = args.file.c_str();
	}
	track = args.track;
	sound = args.sound.c_str();

	FtmDocument doc;
	{
		core::FileIO ftm_io(song, core::IO_READ);
		if (!ftm_io.isReadable())
		{
			printf("Cannot open file\n");
			return 1;
		}

		try
		{
			doc.read(&ftm_io);
		}
		catch (const FtmDocumentException &e)
		{
			fprintf(stderr, "Could not open file: %s\n%s\n", song, e.what());
			exit(1);
		}

		doc.SelectTrack(track-1);
	}

	Session s;

	s.doc = &doc;

	tracker_session = &s;

	initscr();
	noecho();

//	drawwindow(s);

//	signal(SIGWINCH, handle_winch);

	runSong(s, sound);
/*
	char c;
	do
	{
		c = getch();
	}
	while (c != 27 && c != 'q');
*/
	endwin();			/* End curses mode		  */
	return 0;
}
