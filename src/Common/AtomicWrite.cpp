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
        logLev="Debug";
        break;
    case INFO:
        logLev="Info";
        break;
    case WARNING:
        logLev="Warning";
        break;
    case ERROR:
        logLev="Error";
        out = &std::cerr;
        break;
    case FATAL:
        logLev="Fatal Error";
        out = &std::cerr;
        break;
    }

    std::lock_guard<std::mutex> lock(this->mtx);
    (*out) << logLev << ": " << s << std::endl;
}
