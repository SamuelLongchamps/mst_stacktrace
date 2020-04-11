#pragma once
#include "stacktrace.h"
#include "stacktrace.common.h"

#include <sstream>
#include <string>
#include <string_view>
#include <regex>
#include <vector>
#include <algorithm>
#include <charconv>

#include <cxxabi.h>
#include <cstdlib>
#include <cstddef>

#include <unwind.h>
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

frame_content_t parse(frame_t in_address)
{
    Dl_info info;
    if (dladdr(in_address, &info) != 0 && info.dli_sname)
    {
        std::FILE* pipe = nullptr;

        if (info.dli_fbase && info.dli_saddr)
        {
            std::stringstream stream;
            stream << "atos --fullPath -o " << info.dli_fname
                << " -l " << std::hex << info.dli_fbase
                << " " << info.dli_saddr;
            pipe = popen(stream.str().c_str (), "r");
        }

        if (pipe)
        {
            std::stringstream result;
            char buffer[PIPE_BUF];
            while (!feof(pipe))
            {
                if (fgets(buffer, PIPE_BUF, pipe) != nullptr)
                    result << buffer;
            }
            pclose(pipe);

            std::regex r(
                "^(.*)\\(in (.*)\\) \\((.*):([0-9]+)\\)\n?$", std::regex::ECMAScript
            );
            std::smatch m;
            std::string piped_output = result.str();
            if (std::regex_search(piped_output, m, r) && m.size() == 5)
            {
                constexpr static const auto& to_int = [](const std::string& s) -> int {
                    int parsed_line = 0;
                    std::from_chars(
                            s.data(),
                            s.data() + s.size(),
                            parsed_line
                    );
                    return parsed_line;
                };

                return frame_content_t{
                    m[1],
                    m[3],
                    to_int(m[4].str())
                };
            }
            else
            {
                fprintf(stderr, ">>>>> regex_search failed!: [%s]\n", piped_output.c_str());
            }
        }

        // Basic fallback with only the function name
        return frame_content_t{
            demangle(info.dli_sname)
        };
    }
    return frame_content_t{};
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

} // namespace priv

void acquire_stacktrace(stacktrace_t& st)
{
    _Unwind_Backtrace([](struct _Unwind_Context* context, void* arg) -> _Unwind_Reason_Code {
        auto* state = static_cast<stacktrace_t*>(arg);
        state->frames.emplace_back(reinterpret_cast<frame_t>(_Unwind_GetIP(context)));
        return _URC_NO_REASON;
    }, &st);
}

void print_stacktrace(std::FILE* fd, const stacktrace_t& stacktrace)
{
    using namespace mst::priv;

    // We skip the first frame to reach up to acquire_stackframe's caller only
    for (std::size_t i = 1; i < stacktrace.frames.size(); ++i)
    {
       print_frame_content(
           fd,
           parse(stacktrace.frames[i]),
           i == 1 ? "=>" : "^^" // Indicate the direction of the stackframe
        );
    }
}

} // namespace mst