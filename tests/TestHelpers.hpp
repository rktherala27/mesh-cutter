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
}