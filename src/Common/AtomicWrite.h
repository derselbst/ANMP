#include <iostream>
#include <sstream>
#include <mutex>

#define CLOG(LEVEL,MSG) {stringstream ss; ss << MSG; AtomicWrite::getSingleton().write(LEVEL, string(__PRETTY_FUNCTION__) + ": " + ss.str());}
#define LOG(MSG) CLOG(AtomicWrite::LogLevel::INFO, MSG)

class AtomicWrite
{
public:
  enum LogLevel {INFO, WARNING, ERROR, FATAL};
  
    static AtomicWrite& getSingleton();

    void write(std::string s, std::ostream& o=std::cout);
    void write(enum LogLevel, std::string s, std::ostream& o=std::cout);
    
private:
  std::mutex mtx;
  
          AtomicWrite();

        // C++ 11
        // we dont want these
        AtomicWrite(AtomicWrite const&)    = delete;
        void operator=(AtomicWrite const&) = delete;
};

