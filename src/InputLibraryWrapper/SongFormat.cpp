#include "SongFormat.h"

bool operator != (SongFormat const& lhs, SongFormat const& rhs)
{
    return lhs.SampleRate  != rhs.SampleRate ||
           lhs.SampleFormat!= rhs.SampleFormat ||
           lhs.Channels    != rhs.Channels;
}

/**
 * returns bitrate in bit/s
 */
void SongFormat::getBitrate ()
{
// samplerate * sample depth * channels
}

bool SongFormat::IsValid()
{
  return this->SampleRate != 0 && this->Channels != 0 && this->SampleFormat != SampleFormat_t::unknown;
}

