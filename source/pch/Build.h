#pragma once

#ifndef NAI_CLANG
    #ifdef __clang__
        #define NAI_CLANG 1
    #else
        #define NAI_CLANG 0
    #endif
#endif

#ifndef NAI_DEBUG
#error("NAI_DEBUG was not defined to either 0 or 1!");
#endif

#ifndef NAI_RELEASE
#error("NAI_RELEASE was not defined to either 0 or 1!");
#endif

//#define TRACY_ENABLE

#ifdef TRACY_ENABLE
#define TRACY_NO_EXIT
#endif

#define NOMINMAX

#include <Tracy.hpp>
#include <cstddef>
#include <string>
#include <cmath>
#include <cassert>