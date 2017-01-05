#pragma once

#include <exception>
#include <stdexcept>

class NotImplementedException : public std::logic_error
{
public:
    NotImplementedException():std::logic_error("Case or Function not yet implemented") {}
};

class LoopTreeImplementationException : public std::logic_error
{
public:
    LoopTreeImplementationException():std::logic_error("There seems to be an error in Song::buildLoopTree()") {}
};

class NotInitializedException : public std::runtime_error
{
public:
    NotInitializedException():std::runtime_error("Object not initialized, call init() first!") {}
};

class LoopTreeConstructionException : public std::runtime_error
{
public:
    LoopTreeConstructionException():std::runtime_error("The user propably defined illegal (e.g. intersecting) loop points") {}
};

class AssertionException : public std::runtime_error
{
public:
    AssertionException(const char* msg):std::logic_error(msg) {}
};
