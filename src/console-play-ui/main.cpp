#include <stdio.h>
#include "famitracker-core/App.hpp"
#include "famitracker-core/Document.hpp"
#include "famitracker-core/FtmDocument.hpp"
#include "famitracker-core/sound.hpp"
#include "famitracker-core/TrackerController.hpp"
#include "../parse_arguments.hpp"

const char *default_sound="alsa";

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

static void print_help()
{
	printf(
"Usage: app FILE [-t TRACK] [-sr SAMPLERATE] [-sound ENGINE] [--help]\n\n"
"    -t TRACK\n"
"        Select the track number to play. 1 is the first song.\n"
"    -sr SAMPLERATE\n"
"        Set the playback sample rate in herz. Default is 48000.\n"
"    -sound ENGINE\n"
"        Specify which sound engine to use. This will load a module\n"
"        in your PATH named libcore-ENGINE-sound.so. Default is alsa.\n"
"        (eg. -sound jack)\n"
"    --help\n"
"        Print this message\n"
	);
}

static void tracker_update(SoundGen::rowframe_t rf, FtmDocument *doc)
{
	if (!rf.rowframe_changed)
		return;

	int frame = rf.frame;
	printf("[ %02X/%02X: %02X/%02X : ", frame, doc->GetFrameCount()-1, rf.row, doc->getFramePlayLength(frame)-1);
	for (int i = 0; i < doc->GetAvailableChannels(); i++)
	{
		printf("%02X ", doc->GetPatternAtFrame(frame, i));
	}
	printf("]\r");
	fflush(stdout);
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
		print_help();
		return 0;
	}

	if (args.file.empty())
	{
		printf("Please specify a song\n\n");
		print_help();
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
		doc.read(&ftm_io);

		doc.SelectTrack(track-1);
	}

	printf("Name: %s\nArtist: %s\nCopyright: %s\n", doc.GetSongName(), doc.GetSongArtist(), doc.GetSongCopyright());
	printf("Track %u/%u: %s\n", track, doc.GetTrackCount(), doc.GetTrackTitle(track-1));
	printf("BPM: %u [Tempo: %u, Speed: %u]\n\n", doc.GetSongTempo()*6/doc.GetSongSpeed(), doc.GetSongTempo(), doc.GetSongSpeed());
	{
		unsigned int rate = 48000;

		core::SoundSinkPlayback *sink = (core::SoundSinkPlayback*)core::loadSoundSink(sound);
		if (sink == NULL)
		{
			return 1;
		}
		sink->initialize(rate, 1, 150);

		SoundGen *sg = new SoundGen;
		sg->setSoundSink(sink);
		sg->setDocument(&doc);
		sg->setTrackerUpdate(tracker_update);

		sg->trackerController()->startAt(0, 0);
		sg->startTracker();
		sink->blockUntilStopped();
		sink->blockUntilTimerEmpty();

		delete sink;
		delete sg;

		fflush(stdout);
		printf("\n");
	}

	return 0;
}
