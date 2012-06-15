Preview 2
---------

Dan Spencer - June 14, 2012

What's in this preview
----------------------
* A work-in-progress Qt graphical user interface (famitracker-qt)
* A simple player in the console (famitracker-play)
* Support for 2A03/2A07, MMC5, VRC6, *and now FDS*
   (using VRC7 *may* incur copyright issues with using emu2413.c, which is the
   only reason it's not included)


What does/doesn't work in the GUI
---------------------------------
Does
* Playback
* Opening files
* (Basic) Pattern editing, keyboard
* Changing tempo, songs
Doesn't
* Saving files
* Instrument editing
* Channel headers and volume indicators
* Sound scope (why not a button instead?)
* Everything else


Known bugs/limitations
----------------------
Frame view the wrong size on startup (how do you properly set a client size
on a QAbstractScrollView?)
    
enl_track1.ftm, and possibly others, won't play sequences. This results
in sustaining instruments with no effects to them. Resaving it in a newer
version (using the Windows FamiTracker) plays the file just fine.

FamiTracker files below version 0x0200 don't open, as reading files this old
is currently unimplemented.
I'm hard pressed finding files this old, so it's generally not a concern for
preview purposes. Even versions older than 0x0300 are difficult to find.


Bug fixes (bugs which were my fault)
------------------------------------
* One-row latency is eliminated
* Global jump events (Cxx, Bxx, Dxx) precedence rules


For programming geeks
---------------------
* Major reorganization of the underlying architecture
  - Each sound API is its own loadable module (ALSA is libfami-alsa-sound.so).
    This is done to avoid mandatory dependencies if a user doesn't have a
    particular sound API on their system (such as JACK, which is wip).
    The appropriate sound API is loaded dynamically, and the user can override
    the default (alsa) by providing the -sound flag
    (eg. "-sound jack" would attempt to load libfami-jack-sound.so).
  - The core is separated into its own shared library.
    This is done to reuse all the important engine code in multiple UI builds
    of FamiTracker. It also makes it more obvious for hobbyists and developers
    who want to rip the engine where the important code is.


Compiling (requires the source code)
------------------------------------
Requires g++/clang++, libasound2-dev, libboost_thread-dev, libqt4-dev to build.
(Ubuntu/Debian packages)

If you want to test it without installing on your system (recommended):
1. Extract the directory in the .tar.gz into a directory of your choice
2. Navigate to where you extracted the .tar.gz, create "famitracker-bin" and
   "famitracker-install", and navigate to famitracker-bin:
    $ cd /location/of/extracted/source
    $ mkdir famitracker-bin
    $ mkdir famitracker-install
    $ cd famitracker-bin
3. Initialize CMake
    $ cmake -DCMAKE_BUILD_TYPE=Release -DINSTALL_PORTABLE:BOOL=TRUE -DCMAKE_INSTALL_PREFIX=../famitracker-install ../famitracker
    OPTIONAL: If you don't want the Qt GUI, pass -DUI_QT:BOOL=FALSE
4. Make
    $ make
5. Install (packs everything into the famitracker-install directory)
    $ make install
6. Run
    $ cd ../famitracker-install
    $ ./famitracker-qt.sh

To install in /usr/local:
1. Extract the directory in the .tar.gz into a directory of your choice
2. Navigate to where you extracted the .tar.gz, create "famitracker-bin",
   and navigate to famitracker-bin:
    $ cd /location/of/extracted/source
    $ mkdir famitracker-bin
    $ cd famitracker-bin
3. Initialize CMake
    $ cmake -DCMAKE_BUILD_TYPE=Release ../famitracker
4. Make
    $ make
5. Install
    $ sudo make install
    $ sudo ldconfig
6. Run
    $ famitracker-qt


Usage tips
----------
If you find the tracker scrolling is jumpy, try running "pulseaudio -k" if
you use PulseAudio.

