# ANMP
Another Nameless Music Player

## Features

ANMP aims to be a versatile audio player, just as the other hundred thousands out there. In addition it supports:

* gapless playback
* cue sheets
* arbitrary (forward) looping of songs
* easy attempt to implement new formats

In order to achieve gapless playback ANMP handles audio differently than others: Instead of retrieving only small buffers that hold the raw pcm data, ANMP fetches the pcm of the whole file and puts that into memory (well this is at least the case for streamed audio files). Todays computers have enough memory to hold raw pcm of even longer audio files. Uncompressing big audio files can take a long time. Thus filling the pcm buffer is usually done asynchronously. When the next song shall be played, the pcm buffer of the former song is released.

However, ANMP also supports rendering pcm to a small buffer. The method will be used if there is not enough memory available to hold a whole song in memory.


Cue sheets will just add the same song file multiple times to a playlist, but with different file-offsets.

Implementing new formats shall be done by implementing the abstract base class **Song**. By that a wrapper for any library that actually supports this format is written.

## Build Dependencies

* cmake >= 2.8
* gcc >= 4.8.1
* [libcue](https://github.com/lipnitsk/libcue) (only for cuesheet support)
* ALSA
* QT5 (for GUI support only)

#### At least one of the following libraries
(depending on which audio formats ANMP shall be able to play)
* libsndfile (strongly recommended to play most common audio formats)
* [mad](https://sourceforge.net/projects/mad/files/libmad/) >= 0.15.1b
* ffmpeg >= 2.8.6 (specifically libavcodec, libavformat, libavutil, libswresample)
* [lazyusf2](https://gitlab.kode54.net/kode54/lazyusf2)
* [libgme](https://bitbucket.org/mpyne/game-music-emu)
* [vgmstream](https://gitlab.kode54.net/kode54/vgmstream)

## Building from Source
```shell
mkdir build && cd build
cmake ..
make
```

