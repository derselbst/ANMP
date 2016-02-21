#ifndef ALSAOUTPUT_H
#define ALSAOUTPUT_H

#include "IAudioOutput.h"


/**
  * class ALSAOutput
  * 
  */

class ALSAOutput : public IAudioOutput
{
public:

    // Constructors/Destructors
    //  


    /**
     * Empty Constructor
     */
    ALSAOutput ();

    /**
     * Empty Destructor
     */
    virtual ~ALSAOutput ();

};

#endif // ALSAOUTPUT_H
