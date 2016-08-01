
#pragma once


#include <map>


/** @brief a helper class for realizing C# like events
 *
 * register a callback function with operator += and deregister with -=
 *
 * any callback function will always take one parameter, i.e. a void* which can be used for
 * passing around the this pointer, if you dont need it, just register the function with nullptr
 *
 * @code
 * void someCallbackFunction(void* context);
 * void SomeClass::someStaticMemberFunction(void* context);
 * Event<> someEvent;
 *
 * someEvent += std::make_pair(this, &SomeClass::someStaticMemberFunction);
 * someEvent += std::make_pair(nullptr, &someCallbackFunction);
 *
 * someEvent.Fire();
 *
 * someEvent -= this;
 * someEvent -= std::make_pair(nullptr, &someCallbackFunction);
 * ...
 *
 * void someOtherCallbackFunction(void* context, int i, bool b, string s);
 * void SomeOtherClass::someOtherStaticMemberFunction(void* context, int i, bool b, string s);
 * Event<int i, bool b, string s> someOtherEvent;
 *
 * someOtherEvent += std::make_pair(this, &SomeOtherClass::someOtherStaticMemberFunction);
 * someOtherEvent += std::make_pair(someObject, &someOtherCallbackFunction);
 *
 * someOtherEvent.Fire();
 *
 * someOtherEvent -= someObject;
 * someOtherEvent -= this;
 * @endcode
 */
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
