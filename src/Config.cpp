#include "Config.h"

// Constructors/Destructors
//  

Config::Config () {
initAttributes();
}

Config::~Config () { }

//  
// Methods
//  


// Accessor methods
//  


// Other methods
//  

void Config::initAttributes () {
    useLoopInfo = true;
    overridingGlobalLoopCount = -1;
    FramesToRender = 2048;
}

