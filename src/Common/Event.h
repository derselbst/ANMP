
#include <map>



template<typename... Args>
class Event
{    
  std::map<void*, void(*)(void*, Args...)> callbacks;
  
public:
  Event<Args...>& operator+=(std::pair<void*, void(*)(void*, Args...)>);
  Event<Args...>& operator-=(std::pair<void*, void(*)(void*, Args...)>);
  Event<Args...>& operator-=(void*obj);

  void Fire(Args... args);
};

template<typename... Args>
Event<Args...>& Event<Args...>::operator+=(std::pair<void*, void(*)(void*, Args...)> t)
{
    this->callbacks[t.first] = t.second;
    
    return *this;
}

template<typename... Args>
Event<Args...>& Event<Args...>::operator-=(std::pair<void*, void(*)(void*, Args...)> t)
{
    void* obj = t.first;
    
    typename std::map<void*, void(*)(void*, Args...)>::iterator it = this->callbacks.find(obj);
    
    if(it != this->callbacks.end() && it->second == t.second)
    {
        this->callbacks.erase(it);
    }
    
    return *this;
}

template<typename... Args>
Event<Args...>& Event<Args...>::operator-=(void* obj)
{
    this->callbacks.erase(obj);
    
    return *this;
}

template<typename... Args>
void Event<Args...>::Fire(Args... args)
{
    typename std::map<void*, void(*)(void*, Args...)>::iterator it;
    
    for(it = this->callbacks.begin(); it!=this->callbacks.end(); it++)
    {
      it->second(it->first, args...);
    }
    
}