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
#include "Types.h"
#include <cmath>
#include <cassert>
#include "../Utils/DebugHandler.h"

template<class T> constexpr T operator~ (T a) { return (T)~(int)a; }
template<class T> constexpr T operator| (T a, T b) { return (T)((int)a | (int)b); }
template<class T> constexpr T operator& (T a, T b) { return (T)((int)a & (int)b); }
template<class T> constexpr T operator^ (T a, T b) { return (T)((int)a ^ (int)b); }
template<class T> constexpr T& operator|= (T& a, T b) { return (T&)((int&)a |= (int)b); }
template<class T> constexpr T& operator&= (T& a, T b) { return (T&)((int&)a &= (int)b); }
template<class T> constexpr T& operator^= (T& a, T b) { return (T&)((int&)a ^= (int)b); }