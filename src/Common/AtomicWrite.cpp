#include <iostream>

#include "AtomicWrite.h"

AtomicWrite::AtomicWrite()
{
}

AtomicWrite &AtomicWrite::getSingleton()
{
    // guaranteed to be destroyed
    static AtomicWrite instance;

    return instance;
}

void AtomicWrite::write(std::string s, std::ostream &o)
{
    this->write(LogLevel_t::Info, s, o);
}


void AtomicWrite::write(LogLevel_t l, std::string s, std::ostream &o)
{
    std::ostream *out = &o;

    std::string logLev;
    switch (l)
    {
        case LogLevel_t::Debug:
#ifdef NDEBUG
            return;
#endif
            logLev = "Debug";
            break;

        case LogLevel_t::Info:
            logLev = "Info";
            break;

        case LogLevel_t::Warning:
            logLev = "Warning";
            break;

        case LogLevel_t::Error:
            logLev = "Error";
            if (out == &std::cout)
            {
                out = &std::cerr;
            }
            break;

        case LogLevel_t::Fatal:
            logLev = "Fatal Error";
            if (out == &std::cout)
            {
                out = &std::cerr;
            }
            break;
    }

    std::lock_guard<std::mutex> lock(this->mtx);
    (*out) << logLev << ": " << s << std::endl;
}
