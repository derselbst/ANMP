#pragma once

#include <exception>
#include <stdexcept>
#include <string>

class NotImplementedException : public std::logic_error
{
    public:
    NotImplementedException()
    : std::logic_error("Case or Function not yet implemented")
    {
    }
};

class LoopTreeImplementationException : public std::logic_error
{
    public:
    LoopTreeImplementationException()
    : std::logic_error("There seems to be an error in Song::buildLoopTree()")
    {
    }
};

class AssertionException : public std::logic_error
{
    public:
    AssertionException(std::string &msg)
    : std::logic_error(msg)
    {
    }
};


class NotInitializedException : public std::runtime_error
{
    public:
    NotInitializedException()
    : std::runtime_error("Object not initialized, call init() first!")
    {
    }
};

class LoopTreeConstructionException : public std::runtime_error
{
    public:
    LoopTreeConstructionException()
    : std::runtime_error("The user propably defined illegal (e.g. intersecting) loop points")
    {
    }
};

class InvalidVersionException : public std::runtime_error
{
    public:
    InvalidVersionException(uint32_t actual)
    : std::runtime_error("Unknown version: " + std::to_string(actual))
    {
    }
};
