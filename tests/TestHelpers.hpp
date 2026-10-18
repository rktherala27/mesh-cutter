#pragma once

#include <stdexcept>

#ifndef TEST_DATA_DIR
#define TEST_DATA_DIR "tests/data"
#endif

template <typename Fn>
bool expectThrows(Fn &&fn)
{
    try
    {
        fn();
        return false;
    }
    catch (const std::runtime_error &)
    {
        return true;
    }
    catch (...)
    {
        return false;
    }
}

// Returns true if fn throws exactly ExceptionType.
template <typename ExceptionType, typename Fn>
bool expectThrowsAs(Fn &&fn)
{
    try
    {
        fn();
        return false;
    }
    catch (const ExceptionType &)
    {
        return true;
    }
    catch (...)
    {
        return false;
    }
}

// Returns true if fn completes without throwing any exception.
template <typename Fn>
bool expectNoThrow(Fn &&fn)
{
    try
    {
        fn();
        return true;
    }
    catch (...)
    {
        return false;
    }
}

void check(bool condition, const std::string &message)
{
    if (!condition)
    {
        throw std::runtime_error(message);
    }
}

bool pointsAreClose(
    const Vec3 &actual,
    const Vec3 &expected,
    double epsilon)
{
    return std::abs(actual.x - expected.x) <= epsilon &&
           std::abs(actual.y - expected.y) <= epsilon &&
           std::abs(actual.z - expected.z) <= epsilon;
}