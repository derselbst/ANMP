#pragma once

#include <iostream>
#include <mutex>
#include <sstream>

#if defined(__clang__) || defined(__GNUC__) || defined(__GNUG__)
#define INFUNCTION std::string(__PRETTY_FUNCTION__) + ": "
#elif defined(_MSC_VER)
#define INFUNCTION std::string(__FUNCSIG__) + ": "
#else
#define INFUNCTION std::string(__func__) + ": "
#endif

#define LOG_MSG(MSG)     \
    std::stringstream logmsg; \
    logmsg << INFUNCTION << MSG
#define CLOG(LEVEL, MSG)                                        \
    {                                                           \
        LOG_MSG(MSG);                                           \
        AtomicWrite::getSingleton().write(LEVEL, logmsg.str()); \
    }
#define LOG(MSG) CLOG(LogLevel_t::Info, MSG)

#define THROW_RUNTIME_ERROR(MSG)           \
    {                                      \
        LOG_MSG(MSG);                      \
        throw std::runtime_error(logmsg.str()); \
    }


enum class LogLevel_t : uint8_t
{
    Debug,
    Info,
    Warning,
    Error,
    Fatal
};

class AtomicWrite
{
    public:
    static AtomicWrite &getSingleton();

    void write(std::string s, std::ostream &o = std::cout);
    void write(LogLevel_t, const std::string& s, std::ostream &o = std::cout);

    private:
    std::mutex mtx;

    AtomicWrite();

    // C++ 11
    // we dont want these
    AtomicWrite(AtomicWrite const &) = delete;
    void operator=(AtomicWrite const &) = delete;
};
