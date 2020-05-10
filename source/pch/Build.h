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