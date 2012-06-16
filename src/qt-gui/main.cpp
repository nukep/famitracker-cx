#include <stdio.h>
#include <string.h>
#include "gui.hpp"
#include "famitracker-core/App.hpp"

const char *default_sound="alsa";

struct arguments_t
{
	arguments_t()
	// defaults
		: sampleRate(48000), track(0)
	{
		m_sound = NULL;
	}
	~arguments_t()
	{
		if (m_sound != NULL)
			free(m_sound);
	}

	void setSound(const char *s)
	{
		if (m_sound != NULL)
			free(m_sound);

		if (s == NULL)
			m_sound = NULL;
		else
			m_sound = strdup(s);
	}
	const char *sound() const{ return m_sound; }

	unsigned int sampleRate;
	unsigned int track;
private:
	char *m_sound;
};

const char *appname;

static void erase_argv(int &argc, char *argv[], unsigned int at)
{
	if (at >= argc)
		return;

	unsigned int to = argc;
	for (unsigned int i = at; i < to; i++)
	{
		argv[i] = argv[i+1];
	}
	argc--;
}
static const char * pop_argv(int &argc, char *argv[], unsigned int at)
{
	const char *s = argv[at];
	erase_argv(argc, argv, at);
	return s;
}

static void erase_argv(int &argc, char *argv[], unsigned int at, unsigned int count)
{
	for (unsigned int i = 0; i < count; i++)
	{
		erase_argv(argc, argv, at);
	}
}

static void parse_arguments(int ac, char ** const av, arguments_t &a)
{
	// once we read an argument, we'll take it out of argv

	int argc = ac;
	char **argv = new char*[ac+1];
	memcpy(argv, av, sizeof(char*)*(ac+1));

	appname = argv[0];
	erase_argv(argc, argv, 0);

	int idx = 0;

	while (true)
	{
		const char *arg = argv[idx];
		if (arg == NULL)
		{
			// we're done here
			delete[] argv;
			return;
		}
#define ARGSTR(s) const char *s = pop_argv(argc, argv, idx+1); if (s == NULL) return
		if (arg[0] == '-')
		{
			const char *f = arg+1;
			// flag
			if (strcmp(f, "sr") == 0)
			{
				// sample rate
				ARGSTR(s);
				a.sampleRate = atoi(s);
			}
			else if (strcmp(f, "t") == 0)
			{
				// track
				ARGSTR(s);
				a.track = atoi(s);
			}
			else if (strcmp(f, "sound") == 0)
			{
				ARGSTR(s);
				a.setSound(s);
			}
			else
			{
				// unrecognized. ignore
				idx++;
				continue;
			}
			erase_argv(argc, argv, idx);
		}
		else
		{

		}
	}
}

int main(int argc, char *argv[])
{
	gui::init(argc, argv);

	arguments_t args;
	args.setSound(default_sound);

	parse_arguments(argc, argv, args);

	printf("Welcome to FamiTracker!\n");

	gui::init_2(args.sound());

	gui::spin();

	gui::destroy();

	return 0;
}
