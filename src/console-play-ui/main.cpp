#include <stdio.h>
#include "core/App.hpp"
#include "core/Document.hpp"
#include "core/FtmDocument.hpp"
#include "core/sound.hpp"
#include "core/TrackerController.hpp"

const char *default_sound="alsa";

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

static void tracker_update(SoundGen *g)
{
	TrackerController *c = g->trackerController();
	int frame = c->frame();
	printf("[ %02X/%02X: %02X/%02X : ", frame, c->document()->GetFrameCount()-1, c->row(), c->document()->getFramePlayLength(frame)-1);
	for (int i = 0; i < c->document()->GetAvailableChannels(); i++)
	{
		printf("%02X ", c->document()->GetPatternAtFrame(frame, i));
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
		printf("Please specify a song\n");
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

		SoundSinkPlayback *sink = (SoundSinkPlayback*)app::loadSoundSink(default_sound);
		if (sink == NULL)
		{
			return 1;
		}
		sink->initialize(rate, 1, 150);

		SoundGen *sg = new SoundGen;
		sg->setSoundSink(sink);
		sg->setDocument(&doc);
		sg->setTrackerUpdate(tracker_update);

		sg->run();
		delete sg;
		delete sink;

		fflush(stdout);
		printf("\n");
	}

	return 0;
}
