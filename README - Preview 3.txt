Preview 3
---------

Dan Spencer - August 2, 2012

What's in this preview
----------------------
* A work-in-progress Qt graphical user interface (famitracker-qt)
* A simple player in the console (famitracker-play)
* Support for 2A03/2A07, MMC5, VRC6, and FDS
   (using VRC7 *may* incur copyright issues with using emu2413.c, which is the
   only reason it's not included)
* JACK support


What does/doesn't work in the GUI
---------------------------------
Does
* Playback
* Opening, saving files
* (Basic) Pattern editing, keyboard
* Channel headers and volume indicators
* Changing tempo, songs
* Instrument editing
* Sequence editing
* Note auditioning (previewing)
* Basic styles (through View -> Styles)
Doesn't
* Sound scope (why not a button instead?)
* Adding/editing VRC6, FDS instruments
* DPCM previewing
* Everything else


Known bugs/limitations
----------------------
FamiTracker files below version 0x0200 don't open, as reading files this old
is currently unimplemented.
I'm hard pressed finding files this old, so it's generally not a concern for
preview purposes. Even versions older than 0x0300 are difficult to find.

The 2A03 arpeggio sequence has a very high range (-96 to 96), and is hard to use
with the mouse.

No implicit save. Save functionality is redirected to Save As.

Semi-rare crashes may occur when opening/creating a new document.

Note previewing is a bit glitchy. It causes xruns in JACK.


Bug fixes (bugs which were my fault)
------------------------------------
Many, many threading issues
Sequences in older FamiTracker modules (eg. enl_track1.ftm) now play


For programming geeks
---------------------
* More reorganization of the sound architecture
  - Another thread is created for sending update signals to the UI. The timing
    thread accounts for latency, so that even high latency devices will not
    fall out of sync with the UI.
  - Sound Sink stuff is more approachable to callback-based APIs such as JACK.


Compiling (requires the source code)
------------------------------------
Requires g++/clang++, libasound2-dev, libboost-thread-dev, libqt4-dev to build.
libjack-dev is optional.
(Ubuntu/Debian packages)

If you want to test it without installing on your system (recommended):
1. Extract the directory in the .tar.gz into a directory of your choice
2. Navigate to where you extracted the .tar.gz, create "famitracker-bin" and
   "famitracker-install", and navigate to famitracker-bin:
    $ cd /location/of/extracted/source
    $ mkdir famitracker-bin
    $ mkdir famitracker-install
    $ ls
      famitracker  famitracker-bin  famitracker-install
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
    $ ls
      famitracker  famitracker-bin
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


Running JACK
------------
Provided you have JACK installed and set up, simply pass "-sound jack" to
famitracker-play or famitracker-qt.
This has to be done every time you run the application.


Usage tips
----------
If you find the tracker scrolling is jumpy, try running "pulseaudio -k" if
you use PulseAudio.

