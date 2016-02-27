
#include <exception>
#include <stdexcept>

class NotImplementedException : public std::logic_error
{
public:
  NotImplementedException():std::logic_error("Case or Function not yet implemented"){}
}; 

class NotInitializedException : public std::runtime_error
{
public:
  NotInitializedException():std::runtime_error("Object not initialized, call init() first!"){}
}; 
