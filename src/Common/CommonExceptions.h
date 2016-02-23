
#include <exception>
#include <stdexcept>

class NotImplementedException : public std::logic_error
{
public:
  NotImplementedException():std::logic_error("Case or Function not yet implemented"){}
}; 
