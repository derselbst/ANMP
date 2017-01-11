# ANMP
**A**nother **N**ameless **M**usic **P**layer

![ANMP Screenshot](ANMP.png)

## Features

ANMP aims to be a versatile but lightweight audio player, just as the other hundred thousands out there. It is written in C++11. As being only a frontend, ANMP itself doesnt know anything about audioformats and how to decode them. That's why it uses 3rd party libraries to decode them. By using VgmStream, GameMusicEmu, LazyUSF and supporting looped songs natively, ANMP is esp. suited to play various audio formats from video games. Moreover it supports [Looped Midi Tracks](https://github.com/derselbst/ANMP/wiki/MIDI-Customizations).

What ANMP does NOT:
* crawl through your music library, building a huge useless database for all artists, albums, genres, titles, etc., just because the user wants to have a nice overview over what is on his HDD (SSD); the filesystem is our database!
* mess around with any audio files, i.e. no changing of ID3 tags, no writing of ReplayGain or whatever; additional info will always be written to separate files and ONLY then, when the user explicitly says so (e.g. by executing anmp-normalize)
* support ReplayGain tags, since video game codecs don't do (instead it uses a custom approach using helper files to handle loudness normalization independently from audio formats)
* provide a multilingual GUI; everything is done in English!


#### Main Features

* gapless playback
* cue sheets
* arbitrary (forward) looping of songs (i.e. even nested loops)
* unrolling looped MIDI tracks
* easy attempt to implement new formats

ANMP handles audio differently than others: Instead of retrieving only small buffers that hold the raw pcm data, ANMP fetches the pcm of the whole file and puts that into memory. Todays computers have enough memory to hold raw pcm of even longer audio files. Uncompressing big audio files can take a long time, though. Thus filling the pcm buffer is usually done asynchronously. When the next song shall be played, the pcm buffer of the former song is released.

However, ANMP also supports rendering pcm to a small buffer. This method will be used if there is not enough memory available to hold a whole song in memory. This is esp. true, if the user requests infinite playback and the underlying song supports that but doesnt specify any loop points (as it applies to emulated sound formats like the Ultra64 Sound Format (USF)). In this case pcm gets rendered to a small buffer. Whenever this method is used, there will be no seeking within the song possible though.

Cue sheets will just add the same song file multiple times to a playlist, but with different file-offsets.

Implementing new formats shall be done by deriving the abstract base class **Song**. By that a wrapper for any library that actually supports this format is written.

The core of ANMP (i.e. everything in [src/](src/)) is strictly keept **free of any QT5 dependencies**. Makes reuseage a lot easier.

## Get ANMP
See [HERE](https://software.opensuse.org/download.html?project=home%3Aderselbst%3Aanmp&package=anmp) for precompiled packages. Due to a lack of vgmstream and lazyusf on ArchLinux and Debian/Ubuntu, only the RPM packages are built with all features and codec support available for ANMP.

## Build Dependencies

* cmake >= 3.1.0
* a C99 and C++11 compilant compiler (e.g. clang >= 3.5 or gcc >= 4.8)
* Qt5Widgets (highly recommended for GUI support, since there currently is no CLI)

#### At least one of the following codec libraries
(depending on which audio formats ANMP shall be able to play)
* [libsndfile](http://www.mega-nerd.com/libsndfile/) (strongly recommended to play most common audio formats)
* [mad](https://sourceforge.net/projects/mad/files/libmad/) >= 0.15.1b (to play mp3 files)
* [ffmpeg](https://ffmpeg.org) >= 2.8.6 (specifically libavcodec, libavformat, libavutil, libswresample)
* [lazyusf2](https://gitlab.kode54.net/kode54/lazyusf2) (to play Ultra64 Sound Format (usf))
* [libgme](https://bitbucket.org/mpyne/game-music-emu) (Famicom (nsf), SuperFamicon (spc), GameBoy (gbs), etc.)
* [vgmstream](https://gitlab.kode54.net/kode54/vgmstream) (various audio formats from sixth generation video game consoles and following)

#### For audio playback: at least one of the following audio I/O libraries
(if none of them, only WAVE files can be written)
* ALSA (audio playback on Linux only)
* [Jack](http://jackaudio.org/) (low latency audio playback; also requires [libsamplearate](http://www.mega-nerd.com/SRC/))
* [PortAudio](http://www.portaudio.com/) (for crossplatform audio playback support)

### Optional
* [libcue](https://github.com/lipnitsk/libcue) (for cuesheet support)
* [libebur128](https://github.com/jiixyj/libebur128) (for generating loudness normalization files (*.ebur128) using anmp-normalize)
* Qt5OpenGL (for nice blinky audio visualizers)

## Building from Source (Linux)
```shell
mkdir build && cd build
cmake ..
make
```

Also see [[Wiki] About the buildsystem](https://github.com/derselbst/ANMP/wiki/About-the-buildsystem)

## Building from Source (Windows)
You probably want to use Visual Studio to get a native (preferably static) build of ANMP. Decide for one build architecture and one runtime library. Then:

1. Make sure you have built the depending libraries you would like ANMP to use (see above)
2. Instruct cmake where to find each and every of those libraries as well as its include dirs (see below)
3. Get a bunch of library conflict errors
4. Mess around
5. Start again at 1. with different architecture / runtime library

I had several attempts to get ANMP built on Windows, couldnt get it working after spending a bunches of hours though. Static standalone Win32 builds welcome!

```shell
mkdir build && cd build
cmake .. -G "Visual Studio 14 2015" -DENABLE_ALSA=0 -DENABLE_JACK=0 -DLIBSND_LIBRARIES=/path/to/sndfile
```
## TODO
* support loading playlists (.m3u !)
* save settings + playlist on close
* invent a nice way of getting audio groups used (i.e. if one song offers multiple themes, such as underwater and mainland theme)
