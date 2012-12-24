#include <stdio.h>
#include <string.h>
#include "GUI.hpp"
#include "famitracker-core/App.hpp"
#include "../parse_arguments.hpp"
#include "../defaults.hpp"
#ifdef WINDOWS
#	include <Windows.h>
#endif

const char *default_sound=DEFAULT_SOUND;

struct arguments_t
{
	bool help;
	std::string sound;
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

	a.sound = pa.string("sound", default_sound);
}

static void print_help()
{
	printf(
"Usage: app [-sound ENGINE] [--help]\n\n"
"    -sound ENGINE\n"
"        Specify which sound engine to use. This will load a module\n"
"        in your PATH named " SOUNDSINKLIB_FORMAT ". Default is " DEFAULT_SOUND ".\n"
"        (eg. -sound jack)\n"
"    --help\n"
"        Print this message\n",

				"ENGINE"
	);
}


#ifdef WINDOWS

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{

	int argc;
	LPWSTR *wargv = CommandLineToArgvW(GetCommandLineW(), &argc);

	char **argv = new char*[argc];

	for (int i = 0; i < argc; i++)
	{
		int bsz = WideCharToMultiByte(CP_UTF8, 0, wargv[i],-1, 0,0,0,0);
		char *s = new char[bsz];
		argv[i] = s;

		WideCharToMultiByte(CP_UTF8, 0, wargv[i],-1,s,bsz,0,0);
		s[bsz-1] = 0;
	}

	LocalFree(wargv);

	arguments_t args;

	gui::init(argc, argv);

	parse_arguments(argc, argv, args);

	gui::init_2(args.sound.c_str());

	gui::spin();

	gui::destroy();

	for (int i = 0; i < argc; i++)
	{
		delete[] argv[i];
	}
	delete[] argv;
}

#else

int main(int argc, char *argv[])
{
	gui::init(argc, argv);

	arguments_t args;

	parse_arguments(argc-1, argv+1, args);

	if (args.help)
	{
		print_help();
		return 0;
	}

	printf("Welcome to FamiTracker!\n");
	fflush(stdout);

	gui::init_2(args.sound.c_str());

	gui::spin();

	gui::destroy();

	return 0;
}

#endif
