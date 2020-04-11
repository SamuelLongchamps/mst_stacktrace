// Modest header-only stacktrace printer for C++
//
// modest [mod-ist] - having or showing a moderate or humble estimate
// of one's merits, importance, etc.; free from vanity, egotism,
// boastfulness, or great pretensions.
//
// \author Samuel Longchamps
// \license MIT
#pragma once

#include "stacktrace.config.h"

#include <cstdio>

namespace mst
{
    // Opaque type representing a stacktrace
    struct stacktrace_t;

    // Acquire the stacktrace as a heap allocated structure
    void acquire_stacktrace(stacktrace_t& st);

    // Print the stacktrace to the provided stream using the provided
    // previously acquired instance
    void print_stacktrace(std::FILE* fd, const stacktrace_t& st);

    // Print the stacktrace to the provided stream
    MST_STACKTRACE_FORCEINLINE
    void print_stacktrace(std::FILE* fd);
}

#if defined(MST_STACKTRACE_WIN)
    #include "stacktrace.win32.inl"
#elif defined(MST_STACKTRACE_LINUX)
    #include "stacktrace.linux.inl"
#elif defined(MST_STACKTRACE_APPLE)
    #include "stacktrace.mac.inl"
#endif

MST_STACKTRACE_FORCEINLINE
void mst::print_stacktrace(std::FILE* fd)
{
    mst::stacktrace_t st;
    acquire_stacktrace(st);
    print_stacktrace(fd, st);
}