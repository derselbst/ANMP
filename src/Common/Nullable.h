#pragma once

#include <cstddef>

template<typename T>
class Nullable final
{
    template<typename T2>
    friend bool operator==(const Nullable<T2> &op1, const Nullable<T2> &op2);


    template<typename T2>
    friend bool operator==(const Nullable<T2> &op, const T2 &value);

    template<typename T2>
    friend bool operator==(const T2 &value, const Nullable<T2> &op);


    template<typename T2>
    friend bool operator==(const Nullable<T2> &op, nullptr_t nullpointer);

    template<typename T2>
    friend bool operator==(nullptr_t nullpointer, const Nullable<T2> &op);


    public:
    Nullable();
    Nullable(const T &value);
    Nullable(nullptr_t nullpointer);
    const Nullable<T> &operator=(const Nullable<T> &value);
    const Nullable<T> &operator=(const T &value);
    const Nullable<T> &operator=(nullptr_t nullpointer);

    T *operator->();

    public:
    bool hasValue;
    T Value;
};

template<typename T>
Nullable<T>::Nullable()
: hasValue(false), Value(T())
{
}

template<typename T>
Nullable<T>::Nullable(const T &value)
: hasValue(true), Value(value)
{
}

template<typename T>
Nullable<T>::Nullable(nullptr_t nullpointer)
: hasValue(false), Value(T())
{
    (void)nullpointer;
}

template<typename T>
bool operator==(const Nullable<T> &op1, const Nullable<T> &op2)
{
    if (op1.hasValue != op2.hasValue)
    {
        return false;
    }

    if (op1.hasValue)
    {
        return op1.Value == op2.Value;
    }
    else
    {
        return true;
    }
}

template<typename T>
bool operator==(const Nullable<T> &op, const T &value)
{
    if (!op.hasValue)
    {
        return false;
    }

    return op.Value == value;
}

template<typename T>
bool operator==(const T &value, const Nullable<T> &op)
{
    if (!op.hasValue)
    {
        return false;
    }

    return op.Value == value;
}

template<typename T>
bool operator==(const Nullable<T> &op, nullptr_t nullpointer)
{
    (void)nullpointer;
    return !op.hasValue;
}

template<typename T>
bool operator==(nullptr_t nullpointer, const Nullable<T> &op)
{
    (void)nullpointer;
    return !op.hasValue;
}

template<typename T>
const Nullable<T> &Nullable<T>::operator=(const Nullable<T> &value)
{
    hasValue = value.hasValue;
    Value = value.Value;
    return *this;
}

template<typename T>
const Nullable<T> &Nullable<T>::operator=(const T &value)
{
    hasValue = true;
    Value = value;
    return *this;
}

template<typename T>
const Nullable<T> &Nullable<T>::operator=(nullptr_t nullpointer)
{
    (void)nullpointer;
    hasValue = false;
    Value = T();
    return *this;
}

template<typename T>
T *Nullable<T>::operator->()
{
    return (*this).hasValue ? &(*this).Value : nullptr;
}
