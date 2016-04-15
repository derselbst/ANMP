
#include <map>



template<typename... Args>
class Event
{
    
  std::map<void*, void(*)(Args&&...)> callbacks;
  
  Event<Args...>& operator+=(std::pair<const void*, void(*)(Args&&...)>);
  Event<Args...>& operator-=(std::pair<const void*, void(*)(Args&&...)>);
  Event<Args...>& operator-=(void*obj);
};

template<typename... Args>
Event<Args...>& Event<Args...>::operator+=(std::pair<const void*, void(*)(Args&&...)> t)
{
    this->callbacks[t.first] = t.second;
    
    return *this;
}

template<typename... Args>
Event<Args...>& Event<Args...>::operator-=(std::pair<const void*, void(*)(Args&&...)> t)
{
    void* obj = t.first;
    
    typename std::map<void*, void(*)(Args&&...)>::iterator it = this->callbacks.find(obj);
    
    if(it != this->callbacks.end() && it->second == t.second)
    {
        this->callbacks.erase(it);
    }
    
    return *this;
}

template<typename... Args>
Event<Args...>& Event<Args...>::operator-=(void*obj)
{
    this->callbacks.erase(obj);
    
    return *this;
}
  
