#include <stdio.h>
#include "famitracker-core/App.hpp"
#include "famitracker-core/Document.hpp"
#include "famitracker-core/FtmDocument.hpp"
#include "famitracker-core/sound.hpp"
#include "famitracker-core/TrackerController.hpp"

const char *default_sound="alsa";

static void tracker_update(SoundGen::rowframe_t rf, FtmDocument *doc)
{
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
	const char *song;
	int track = 1;

	if (argc <= 1)
	{
		printf("Please specify a song\n\n");
		printf("Usage: %s FILE [TRACK]\n", argv[0]);
		return 1;
	}
	else
	{
		song = argv[1];
	}
	if (argc >= 3)
	{
		track = atoi(argv[2]);
	}

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

		core::SoundSinkPlayback *sink = (core::SoundSinkPlayback*)core::loadSoundSink(default_sound);
		if (sink == NULL)
		{
			return 1;
		}
		sink->initialize(rate, 1, 150);

		SoundGen *sg = new SoundGen;
		sg->setSoundSink(sink);
		sg->setDocument(&doc);
		sg->setTrackerUpdate(tracker_update);

		sg->start();
		sink->blockUntilStopped();

		delete sg;
		delete sink;

		fflush(stdout);
		printf("\n");
	}

	return 0;
}
