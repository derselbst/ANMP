#ifndef TYPES_H
#define TYPES_H

#include <cstdint>

typedef void pcm_t; 


typedef enum LoopType
{
  Forward,
  Backward,
  Alternating
} LoopType_t;
 
 
typedef struct loop
{
  // frame to start the loop
  uint32_t start;
  
  // frame to end the loop
  // this frame WILL be played!
  uint32_t stop;
  
  // how many times this loop is being played
  // zero indicates playing forever
  uint32_t count;
  
  LoopType_t type = LoopType::Forward;
  
} loop_t;


typedef enum AudioDriver
{
  AutoDetect,
  ALSA,
  JACK,
} AudioDriver_t;

 
typedef enum SampleFormat
{
  unknown,
  int16,
  float32,
} SampleFormat_t;

#endif