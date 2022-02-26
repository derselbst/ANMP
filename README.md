# ANMP
**A**nother **N**ameless **M**usic **P**layer

[![Build Status](https://travis-ci.org/derselbst/ANMP.svg?branch=master)](https://travis-ci.org/derselbst/ANMP/branches) [![Project Stats](https://www.openhub.net/p/ANMP/widgets/project_thin_badge?format=gif)](https://www.openhub.net/p/ANMP) [![SonarCloud](https://sonarcloud.io/images/project_badges/sonarcloud-white.svg)](https://sonarcloud.io/dashboard?id=derselbst_ANMP)

![ANMP Screenshot](ANMP.png)

## Features

ANMP aims to be a versatile but lightweight audio player, just as the other hundred thousands out there. It is written in C++11. As being only a frontend, ANMP itself doesnt know anything about audio formats and how to decode them. That's why it uses 3rd party libraries to decode them. By using VgmStream, GameMusicEmu, LazyUSF and supporting looped songs natively, ANMP is esp. suited to play various audio formats from video games. Moreover it supports [Looped Midi Tracks](https://github.com/derselbst/ANMP/wiki/MIDI-Customizations).

#### Main Features

* muting multichannel audio files
* gapless playback (for most streamed audio formats)
* arbitrary (forward) looping of songs (i.e. even nested loops)
* synthesizing MIDI files using fluidsynth
  * multi-channel audio rendering
  * overlapping notes do not kill each other
  * unrolling [looped MIDI tracks](https://github.com/derselbst/ANMP/wiki/MIDI-Customizations)
  * volume response compatible to N64's software synth
  * supports an IIR lowpass filter compatible to Rareware's N64 games (Conkers Bad Fur Day, Jet Force Gemini)
* native support for the [Ultra64 **C**ompressed **M**IDI **F**ormat](https://github.com/derselbst/ANMP/wiki/Reproducing-N64-OSTs-accurately#ultra64-compressed-midi-format--soundfonts) (.cmf files) as used in many games (used by Rareware and others)
* support for cue sheets
* support for audio normalization based on EBU R 128
* exposes basic commands via D-Bus

#### ANMP does NOT...

* crawl through your music library, building a huge useless database for all artists, albums, genres, titles, etc., just because the user wants to have a nice overview over what is on his HDD (SSD); the filesystem is our database!
* mess around with any audio files, i.e. no changing of ID3 tags, no writing of ReplayGain or whatever; additional info will always be written to separate files and ONLY then, when the user explicitly says so (e.g. by executing anmp-normalize)
* support ReplayGain tags, since video game codecs don't do (instead it uses a custom approach using helper files to handle loudness normalization independently from audio formats)
* provide a multilingual GUI; everything is done in English!

ANMP handles audio differently than others: Instead of retrieving only small buffers that hold the raw PCM data, ANMP fetches the PCM of the whole file and puts that into memory. Todays computers have enough memory to hold raw PCM of even longer audio files. Uncompressing big audio files can take a long time, though. Thus filling the PCM buffer is usually done asynchronously. When the next song shall be played, the PCM buffer of the former song is released.

However, ANMP also supports rendering PCM to a small buffer. This method will be used if there is not enough memory available to hold a whole song in memory. This is esp. true, if the user requests infinite playback and the underlying song supports that but doesnt specify any loop points (as it applies to emulated sound formats like the Ultra64 Sound Format (USF)). In this case PCM gets rendered to a small buffer. Whenever this method is used, there will be no seeking within the song possible though.

Cue sheets will just add the same song file multiple times to a playlist, but with different file-offsets.

Implementing new formats shall be done by deriving the abstract base class **Song**. By that a wrapper for any library that actually supports this format is written.

The core of ANMP (i.e. everything in [src/](src/)) is strictly keept **free of any QT5 dependencies**. Makes reuseage a lot easier.

## Get ANMP
See [HERE](https://software.opensuse.org/download.html?project=home%3Aderselbst%3Aanmp&package=anmp) for precompiled packages. Due to a lack of vgmstream and lazyusf on ArchLinux and Debian/Ubuntu, only the RPM packages are built with all features and codec support available for ANMP.

## Build Dependencies

* cmake >= 3.1.0
* a C99 and C++11 compilant compiler (e.g. clang >= 3.5 or gcc >= 4.8)
* Qt5Widgets and Qt5DBus (highly recommended for GUI support, since there currently is no CLI)

#### At least one of the following codec libraries
(depending on which audio formats ANMP shall be able to play)
* [libsndfile](http://www.mega-nerd.com/libsndfile/) (strongly recommended to play most common audio formats)
* [mad](https://sourceforge.net/projects/mad/files/libmad/) >= 0.15.1b (to play mp3 files)
* [ffmpeg](https://ffmpeg.org) >= 3.1.0 (specifically libavcodec, libavformat, libavutil, libswresample)
* [lazyusf2](https://gitlab.kode54.net/kode54/lazyusf2) (to play Ultra64 Sound Format (usf))
* [libgme](https://bitbucket.org/mpyne/game-music-emu) (Famicom (nsf), SuperFamicon (spc), GameBoy (gbs), etc.)
* [vgmstream](https://gitlab.kode54.net/kode54/vgmstream) (various audio formats from sixth generation video game consoles and following)
* [aopsf](https://gitlab.kode54.net/kode54/aopsf) (Portable Sound Format (PSF1 and PSF2))
* [modplug](https://github.com/Konstanty/libmodplug) or [libopenmpt](https://lib.openmpt.org/libopenmpt/) (Tracker formats, like MOD, IT, XM, ...)
* [fluidsynth](https://github.com/FluidSynth/fluidsynth) and [libsmf](https://sourceforge.net/projects/libsmf/) (synthesize and play MIDI files)

#### For audio playback: at least one of the following audio I/O libraries
(if none of them, only WAVE files can be written)
* ALSA (audio playback on Linux only)
* [Jack](http://jackaudio.org/) (low latency audio playback; also requires [libsamplearate](http://www.mega-nerd.com/SRC/) >= 0.1.9)
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

## TODO
* support loading playlists (.m3u !)
* serialize playlist to file on close, restore on open
* support surround and quadraphonic files

## Legal
ANMP is licensed under the terms of the *GNU GENERAL PUBLIC LICENSE Version 2*, see [LICENSE](LICENSE).


Copyright (C) 2016-2022 Tom Moebert (derselbst)
