Preview remarks
---------------

Dan Spencer - June 7, 2012

So, a pet project of mine for the past 12 days or so has been trying to port
FamiTracker to Linux. Given the entangling labyrinth of source code, I wasn't
sure if I would follow through. Well, I decided to try anyway.
This is my first porting project, and doing this port has been a valuable
learning experience.

I'm releasing a preview for two reasons.
* To receive useful and constructive feedback from users and developers
* To warm people up to a Linux release instead of (completely) surprising
  everyone in the end.

A git tree is available so anyone can view the steps taken. The master branch
includes a work in progress prototype of the Qt interface.


Using the preview
-----------------
Arguments: ./famitracker FILE [track number]

The preview will playback sound using ALSA in an endless loop.

One could enable WAV exporting by going into main.cpp and uncommenting the
"#define WAVE 120" line. It'll create out.wav in your working directory.


Frequently Asked Questions (by nobody)
--------------------------------------

Why? Wine runs Famitracker just fine!

Except when it doesn't. Others have reported success, but I almost always get
sound buffer underrun errors. This effectively makes Famitracker only good for
exporting WAVs and NSFs. Wine is not flawless, and API bugs are very common.
Wine shouldn't be considered an adequate solution for anything, but only as a
last resort. In the end, I think writing a Linux version is arguably easier than
getting Wine to work. :|


Wouldn't you have to rewrite everything?

Sort of. I cherry picked large portions of the original source, although a TON
of it had to be rewritten. I had to work around MFC object paradigms and the
Win32 API, which in itself was an interesting adventure. The UI portion has to
be rewritten from scratch, which is in progress using Qt.


Nobody uses Linux. Just use Windows like everyone else!

That's not a question, and please calm down. I think choice is a good thing,
and there are users who'd rather not have to dual boot/virtualize just to use
a bloody music program.
Chiptune is a particularly geeky subculture, so there's bound to be more Linux
users out there who'd care for a FamiTracker port. The figure's probably as high
as 5-15%. Or something.
And hey, even if nobody did use Linux, a Mac version should be right around
the corner.


Might jsr have a problem accepting your code?

I'm not currently pushing to get this accepted by jsr or by anybody. I'm simply
solving my own problem.
Because the code varies so much, I recognize the problems in having this
incorporated into the mainline FamiTracker. However, I'd certainly welcome it.


Objectives and todo
-------------------
- Get Famitracker running natively on Linux (duh)
  - Ubuntu/Fedora packages (<1 MB)
  - Other .tar.gz package, bundled with Qt4 and libstdc++ (~6-7 MB)
- Build on Windows and OS X. I don't have a Mac, so someone else can do this.
- Allow use of the player in open source projects
- Create a command line interface for simple jobs
  - Exporting to WAV/NSF
  - Playback
  - Piping sound to stdout
    (eg. $ famitracker -p song.ftm | thisprogramplaysaudio)
- Remote control interface (through named pipe or stdin)
- PulseAudio, SDL audio, Jack playback


Source code/maintenance objectives
----------------------------------
- Allow the source to be reasonably portable on Windows and Mac OS X
- Make the source more modular. Make it easy for an external project to rip out
    the .ftm reader and player, so that they could theoretically use them.
- Separate the UI code from logic code. The UI can interact with the logic, but
    the logic shall never interact with the UI.
  - An exception to the rule is callbacks to the UI.
  - The point is that the logic should be able to work without UI cruft.
- Make the code platform agnostic where it counts. If the logic requires an OS
    call, it should be interfaced through a wrapper.
    The original source is actually not terrible in this regard. There
    are occasional MFC references (such as CMutex and CString) that need
    replaced.
- Clean up the source syntax.
  - It's mostly me imposing my preferences, but it's also a readability thing.
  - This is by no means final. I'll probably change this if there are
      objections by anyone who cares.
  - Eliminate all hungarian notation (eg. iInteger or pPointer).
    - CClass should become Class, m_pPointer should become m_pointer.
  - All classes and types will be in UpperCamelCase.
  - All methods and variable names will be in lowerCamelCase.
  - Braces get their own line.
  - Postfix instead of prefix. i++ instead of ++i, except for iterators or
      assignments.
  - Tidy up for loops. "i" will be local, and use a postfix operator
      for (i = 0; i < n+1; ++i) should become for (int i = 0; i < n+1; i++)
- Clean up and document my contributions
- Rewrite the document reader. The current reader is full of opportunities for
    undefined behavior if the file is corrupt.

Compiling
---------
CMake, g++/clang++ and libasound2-dev are required to build the preview.

1. Create a new directory outside of the source directory and navigate to it
   (eg. $ mkdir famitracker-bin; cd famitracker-bin)
2. cmake -DCMAKE_BUILD_TYPE=Release ../famitracker
3. make


Known bugs/limitations in the preview
-------------------------------------
Files using the VRC7 or FDS expansion chips won't play, as they're not yet
implemented. The implemented chips are: 2A03/2A07, MMC5, VRC6.

Playback doesn't halt (yet).

enl_track1.ftm, and possibly others, won't play sequences. This results
in sustaining instruments with no effects to them. Resaving it in a newer
version (using the Windows FamiTracker) plays the file just fine.

Jump events (Bxx and Dxx) don't follow precedence rules if they're on the same
row (Bxx always wins).

FamiTracker files below version 0x0200 don't open, as reading files this old
is currently unimplemented.
I'm hard pressed finding files this old, so it's generally not a concern for
preview purposes. Even versions older than 0x0300 are difficult to find.

The playback monitor isn't totally synchronized with the music, being a few
rows off. It's more noticable with slower songs. (I think) it's an ALSA
buffering issue I don't quite know how to address at the moment.

