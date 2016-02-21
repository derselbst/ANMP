
#ifndef CONFIG_H
#define CONFIG_H

#include <string>

/**
  * class Config
  * 
  */

/******************************* Abstract Class ****************************
Config does not have any pure virtual methods, but its author
  defined it as an abstract class, so you should not use it directly.
  Inherit from it instead and create only objects from the derived classes
*****************************************************************************/

class Config
{
public:

    // Constructors/Destructors
    //  


    /**
     * Empty Constructor
     */
    Config ();

    /**
     * Empty Destructor
     */
    virtual ~Config ();

    // Static Public attributes
    //  

    static AudioDriver_t audioDriver;
    static bool useLoopInfo;
    static int overridingGlobalLoopCount;
    static const unsigned int FramesToRender;
    // Public attributes
    //  


protected:

    // Static Protected attributes
    //  

    // Protected attributes
    //  

public:

protected:

public:

protected:


private:

    // Static Private attributes
    //  

    // Private attributes
    //  

public:

private:

public:

private:


    void initAttributes () ;

};

#endif // CONFIG_H
