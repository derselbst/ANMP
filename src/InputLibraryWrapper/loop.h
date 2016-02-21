 
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