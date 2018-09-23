# Arctracker
## The Archimedes modfile player

This program is designed to play modfiles that have been created using the
Tracker and Desktop Tracker programs that run on the Acorn Archimedes and
compatible range of microcomputers.

Both Tracker and Desktop Tracker are programs that work in the same manner as
the old Ultimate Soundtracker program by Karsten Obarski and all its myriad
clones that run on the Commodore Amiga.  However, both Tracker and Desktop
Tracker have their own file formats and so none of the existing modfile players
available for Linux (or Windows, to the best of my knowledge) will play them.

This program existed for two reasons:

1. I wanted to be able to play all my old Tracker and Desktop Tracker modfiles
on my Linux machine, and

1. I wanted to write an advanced tracker editor of my own for Linux and this
seemed like a useful exercise to try my hand at first.

Well, 2 never happened and nowadays [Renoise](https://www.renoise.com/) does all that I could ever have
imagined and much more besides. More recently this program has taken on a new
lease of life as a refactoring project.

### Build prerequisites

In this day and age Linux distributions have all dropped the Open Sound System.
For ALSA playback you need to have the ALSA header files installed. For example
on Debian-based distributions, do:

    apt-get install libasound2-dev

If you are using Macos, you need to have PortAudio installed instead. If you
use homebrew then you can do:

    brew install portaudio

If you are building from master, rather than a release tarball, you need to
have GNU automake and autoconf installed. Assuming that requirement, run the
following command:

    autoreconf -vfi

### How to build it

Run the configure script:

    ./configure

You can now build the program with:

    make arctracker

and install it with:

    sudo make install

### How to run it

Currently, the program runs from the command line only.  It is invoked like
this:

    arctracker [options] <modfile>

where `modfile` is the filename of the Tracker or Desktop Tracker modfile to
be played, and `options` may be any or none of the following:

| Short    | Long            | Description |
| -------- | --------------- | ----------- |
| `-h`     | `--help`        | Print usage information and exit. |
| `-i`     | `--info`        | Print information about the module and exit. |
| `-l`     | `--loop`        | Continually repeat the modfile until the program is interrupted by ctrl-c. The default is not to repeat. Looping is disabled when writing to a WAV file. |
| `-p`     | `--pianola`     | Print the pattern information to stdout as the modfile is playing in the manner of an old pianola roll. |
| `-vn`    | `--volume=n`    | Software-scale the volume where 1 ≤ _n_ ≤ 255. When playing modfiles with more than 4 channels (Desktop Tracker modfiles can have up to 16) a volume greater than 64 may cause clipping, which sounds distorted. If this happens then use a lower volume. |
| `-c`     | `--clip-warn`   | Indicate when clipping occurs by printing a bang character to stdout. |
| `-ofile` | `--output=file` | Write audio to named _file_ in WAV format instead of playing back through the audio system. |

### How to hack it

To import the code into CLion then do either of these first as appropriate:

    cp CMakeLists.txt.macos CMakeLists.txt
    cp CMakeLists.txt.linux CMakeLists.txt

Now you can import it as an existing CMake project.

-----------------------------------------------------------------
Arctracker is copyright (c) Richard Wild 2003, 2004, 2005 & 2018.
