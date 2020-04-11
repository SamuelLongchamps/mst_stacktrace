#pragma once
#include "stacktrace.h"


namespace mst
{

typedef const void* frame_t;
struct stacktrace_t
{
    void* out_frames[MIN_FRAME_COUNT];
};

void acquire_stacktrace(stacktrace_t& st)
{
    ::RTLCaptureStackBackTrace(1, MIN_FRAME_COUNT, st.out_frames);
}


void print_stacktrace(std::FILE* fd, const stacktrace_t& stacktrace)
{
    // TODO
}

}