Preview 4
---------

Dan Spencer - January 3, 2013
http://code.google.com/p/famitracker-cx/

What's in this preview
----------------------
* Compatiblity with FamiTracker v0.4.1
* An ncurses player for terminal nerds
* Support for VRC7
  * The license for emu2413.c (zlib) was obtained from the Mednafen project
* Improved GUI
  * Recent files
  * Module properties


What doesn't work in the GUI
---------------------------------
* Sound scope
* Editing FDS, VRC7 instruments
* DPCM previewing
* History (undo/redo)
* Miscellaneous pattern tools
* Making lines in the sequence editor
* The 2A03 arpeggio sequence has a very high range (-96 to 96), and is hard to
  use with the mouse.


Known bugs/limitations
----------------------
* FamiTracker files below version 2.00 don't open, as reading files this old
  is currently unimplemented.
  I'm hard pressed finding files this old, so it's generally not a concern for
  preview purposes. Even versions older than 3.00 are difficult to find.
* No implicit save. Save functionality is redirected to Save As.
* Crashes may occur when opening/creating a new document. This tends to happen
  while the song is still playing.
* Note previewing is a bit glitchy. It causes xruns in JACK.
* Hacked .ftm's with multiple expansion chips will crash the program.
* Sound output is very buggy. Note previewing may make the program unresponsive.
  Another overhaul of the sound architecture is planned, and the process will
  be documented on Google Code.


Bug fixes (bugs which were my fault)
------------------------------------
* Control panel no longer disappers randomly in some WM's when moving the main
  window.
* Some sound code fixes (which, with the upcoming sound overhaul, will end up
  becoming irrelevant anyway)
* .ftm read errors are displayed instead of crashing the program


Compiling (requires the source code)
------------------------------------
Requires g++/clang++, libasound2-dev, libboost-thread-dev, libqt4-dev to build.
libjack-dev and libncurses5-dev are optional.
(Ubuntu/Debian packages)

### If you want to test it without installing on your system (recommended) ###

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


### To install in /usr/local ###

1. Extract the directory in the .tar.gz into a directory of your choice

2. Navigate to where you extracted the .tar.gz, create "famitracker-bin",
   and navigate to famitracker-bin:

        $ cd /location/of/extracted/source
        $ mkdir famitracker-bin
        $ ls
          famitracker  famitracker-bin
        $ cd famitracker-bin"

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
If you find the tracker scrolling is laggy or jumpy, try running "pulseaudio -k"
if you use PulseAudio.

