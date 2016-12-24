#include <iostream>

#include "AtomicWrite.h"

AtomicWrite::AtomicWrite()
{

}

AtomicWrite& AtomicWrite::getSingleton()
{
    // guaranteed to be destroyed
    static AtomicWrite instance;

    return instance;
}

void AtomicWrite::write(std::string s, std::ostream& o)
{
    this->write(INFO, s, o);
}


void AtomicWrite::write(enum LogLevel l, std::string s, std::ostream& o)
{
    std::ostream* out = &o;

    std::string logLev;
    switch(l)
    {
    case DEBUG:
#ifdef NDEBUG
        return;
#endif
        logLev="Debug";
        break;
        
    case INFO:
#ifdef NDEBUG
        return;
#endif
        logLev="Info";
        break;
        
    case WARNING:
        logLev="Warning";
        break;
        
    case ERROR:
        logLev="Error";
        if(out == &std::cout)
        {
            out = &std::cerr;
        }
        break;
        
    case FATAL:
        logLev="Fatal Error";
        if(out == &std::cout)
        {
            out = &std::cerr;
        }
        break;
    }

    std::lock_guard<std::mutex> lock(this->mtx);
    (*out) << logLev << ": " << s << std::endl;
}
