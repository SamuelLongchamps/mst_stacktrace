#pragma once
#include "stacktrace.h"
#include "stacktrace.common.h"

#include <optional>
#include <string>
#include <vector>
#include <algorithm>
#include <cxxabi.h>
#include <cstdlib>
#include <cstddef>

#include <unwind.h>
#include <backtrace.h>
#include <dlfcn.h>


namespace mst
{

typedef const void* frame_t;
struct stacktrace_t
{
    stacktrace_t()
    {
        frames.reserve(priv::FRAME_ALLOC_SIZE);
    }
    std::vector<frame_t> frames;
};

namespace priv
{

struct frame_content_t
{
    std::string function;
    std::string filename;
    int lineno = 0;
};

inline std::string demangle(const char* name)
{
    std::string result;

    int status = 0;
    char* demangled_name = abi::__cxa_demangle(name, nullptr, nullptr, &status);

    result.assign(demangled_name != nullptr && status == 0 ?
        demangled_name :
        name
    );

    return result;
}

frame_content_t parse(frame_t address)
{
    // Support only the calling thread and set a local value
    backtrace_state* state = backtrace_create_state(
        nullptr,
        0,
        [](void *data, const char *msg, int errnum) {},
        0
    );

    if (!state)
    {
        // Simple fallback
        Dl_info info;
        return dladdr(address, &info) != 0 && info.dli_sname ?
            frame_content_t { demangle(info.dli_sname) } :
            frame_content_t{};
    }

    using data_t = std::optional<frame_content_t>;
    data_t data;
    backtrace_pcinfo(
        state,
        reinterpret_cast<uintptr_t>(address),
        [](void *data, uintptr_t, const char *filename, int lineno, const char *function) -> int {
            // We have at least some data
            if (function || filename || lineno != 0)
            {
                auto& d = *static_cast<data_t*>(data);
                d.emplace(frame_content_t{
                    function ? demangle(function) : std::string(),
                    filename ? std::string(filename) : std::string(),
                    lineno
                });
            }
            return 0;
        },
        [](void*, const char*, int) {},
        &data
    )
    ||
    backtrace_syminfo(
        state,
        reinterpret_cast<uintptr_t>(address),
        [](void *data, uintptr_t, const char *symname, uintptr_t, uintptr_t) {
            if (symname)
            {
                auto& d = *static_cast<data_t*>(data);
                if (d.has_value())
                {
                    if (d.value().function.empty())
                    {
                        d.value().function = symname ?
                            demangle(symname) :
                            std::string();
                    }
                }
                else
                {
                    d.emplace(frame_content_t{
                        symname ?
                            demangle(symname) :
                            std::string()
                    });
                }
            }
        },
        [](void*, const char*, int) {},
        &data
    );

    return data.has_value() ? data.value() : frame_content_t{};
}

inline void print_frame_content(
    std::FILE* fd,
    const frame_content_t& fc,
    const char* arrow
    )
{
    using namespace mst::priv::console;

    if (fc.filename.empty())
    {
        // Short version
        fprintf(fd, "%s %s%s%s\n",
            arrow,
            cyan, fc.function.empty() ? "<unknown>" : fc.function.c_str(), NC
        );
    }
    else
    {
        // Long version
        fprintf(fd, "%s %s%s%s\n   [%s:%s%d%s]\n",
            arrow,
            cyan, fc.function.empty() ? "<unknown>" : fc.function.c_str(), NC,
            fc.filename.empty() ? "<unknown>" : fc.filename.c_str(),
            cyan, fc.lineno, NC
        );
    }
}

} // priv

void acquire_stacktrace(stacktrace_t& st)
{
    _Unwind_Backtrace([](struct _Unwind_Context* context, void* arg) -> _Unwind_Reason_Code {
        auto* state = static_cast<stacktrace_t*>(arg);
        state->frames.emplace_back(reinterpret_cast<frame_t>(_Unwind_GetIP(context)));
        return _URC_NO_REASON;
    }, &st);
}

void print_stacktrace(std::FILE* fd, const stacktrace_t& st)
{
    using namespace mst::priv;

    // We skip the first frame and the two lasts ones:
    // * 1st is this file's AcquireStackTrace function
    // * 2 last frames are before __libc_start_main
    const std::size_t size = std::min(
        stacktrace.frames.size(),
        stacktrace.frames.size() - 2
    );

    for (std::size_t i = 1; i < size; ++i)
    {
       print_frame_content(
           fd,
           parse(st.frames[i]),
           i == 1 ? "=>" : "^^" // Indicate the direction of the stackframe
        );
    }
}

} // mst