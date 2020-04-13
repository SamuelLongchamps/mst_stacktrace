#pragma once

#if defined(_WIN32) || defined(_WIN64)
    #define MST_STACKTRACE_WIN
    #define MST_STACKTRACE_FORCEINLINE __forceinline
#elif defined(__linux__)
    #define MST_STACKTRACE_LINUX
    #define MST_STACKTRACE_FORCEINLINE __attribute__((always_inline)) inline
#elif defined(__APPLE__)
    #define MST_STACKTRACE_APPLE
    #define MST_STACKTRACE_FORCEINLINE __attribute__((always_inline)) inline
#else
    #error mst::stacktrace Unsupported platform
#endif

