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


void AtomicWrite::write(enum LogLevel, std::string s, std::ostream& o)
{
  // C++11 only
  // mtx gets locked, and will be unlocked when lock is destroyed
  std::lock_guard<std::mutex> lock(this->mtx); 
  o << s;
}
