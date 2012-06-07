#include <stdio.h>
#include "FtmDocument.hpp"
#include "TrackerController.hpp"
#include "wavoutput.hpp"
#include "alsa.hpp"

class FileIO : public IO
{
public:
	FileIO(const char *filename, const char *mode)
	{
		f = fopen(filename, mode);
	}

	Quantity read(void *buf, Quantity sz)
	{
		if (f == NULL)
			return 0;
		return fread(buf, 1, sz, f);
	}

	Quantity write(const void *buf, Quantity sz)
	{
		if (f == NULL)
			return 0;
		return fwrite(buf, 1, sz, f);
	}
	bool seek(int offset, SeekOrigin o)
	{
		if (f == NULL)
			return false;
		fseek(f, offset, SEEK_SET);
		return true;
	}
	bool isReadable()
	{
		return f != NULL;
	}
	bool isWritable()
	{
		return f != NULL;
	}

	~FileIO()
	{
		if (f != NULL)
		{
			fclose(f);
		}
	}
	FILE *f;
};

struct arguments_t
{
	arguments_t()
	// defaults
		: launch_gui(true), soundRate(48000), track(0)
	{
	}

	bool launch_gui;
	unsigned int soundRate;
	unsigned int track;
};

const char *appname;

static void erase_argv(int &argc, char *argv[], unsigned int at, unsigned int count)
{
	unsigned int to = argc - count;
	for (unsigned int i=at;i<to;i++)
	{
		argv[i] = argv[i+count];
	}
	argc -= count;
}

static void parse_arguments(int &argc, char *argv[], arguments_t &a)
{
	// once we read an argument, we'll take it out of argv
	// modifying argv[] is allowed, so don't get too paranoid

	appname = argv[0];
	erase_argv(argc, argv, 0, 1);
}

static void tracker_update(SoundGen *g)
{
	TrackerController *c = g->trackerController();
	int frame = c->frame();
	printf("[ %02X: %02X/%02X : ", frame, c->row(), c->document()->GetPatternLength());
	for (int i=0;i<c->document()->GetAvailableChannels();i++)
	{
		printf("%02X ", c->document()->GetPatternAtFrame(frame, i));
	}
	printf("]\r");
	fflush(stdout);
}

int main(int argc, char *argv[])
{
	printf("Welcome to FamiTracker!\n");

	arguments_t a;
	parse_arguments(argc, argv, a);

	const char *song;
	int track = 1;

	if (argc == 0)
	{
		printf("Please specify a song\n");
		return 1;
	}
	else
	{
		song = argv[0];
	}
	if (argc >= 2)
	{
		track = atoi(argv[1]);
	}

	FtmDocument doc;
	{
		FileIO ftm_io(song, "rb");
		if (!ftm_io.isReadable())
		{
			printf("Cannot open file\n");
			return 1;
		}
		doc.read(&ftm_io);

		doc.SelectTrack(track-1);
	}

	printf("Name: %s\nArtist: %s\nCopyright: %s\n", doc.GetSongName(), doc.GetSongArtist(), doc.GetSongCopyright());
	printf("Track %u/%u: %s\n", track, doc.GetTrackCount(), doc.GetTrackTitle(track-1));
	printf("BPM: %u [Tempo: %u, Speed: %u]\n\n", doc.GetSongTempo()*6/doc.GetSongSpeed(), doc.GetSongTempo(), doc.GetSongSpeed());
	{
		unsigned int rate = 48000;

//#define WAVE

#ifdef WAVE
		FileIO wav_io("out.wav", "wb");
		WavOutput as(&wav_io, 1, rate);
#else
		AlsaSound as;
		as.initialize(rate, 1, 80);
#endif

		SoundGen sg;
		sg.setSoundSink(&as);
		sg.setDocument(&doc);
		sg.setTrackerUpdate(tracker_update);

		sg.run();
#ifdef WAVE
		as.finalize();
#endif
	}

	return 0;
}
