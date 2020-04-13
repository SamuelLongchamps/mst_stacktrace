#pragma once
#include "stacktrace.h"
#include "stacktrace.common.h"

#include <dbghelp.h>

#include <sstream>
#include <vector>

#pragma comment(lib, "kernel32")
#pragma comment(lib, "Dbghelp")

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

namespace priv {

WORD begin_color(HANDLE outfd_handle, WORD color)
{
    WORD oldcolor = 0;
    if (outfd_handle)
    {
        CONSOLE_SCREEN_BUFFER_INFO buffer_info;
        GetConsoleScreenBufferInfo(outfd_handle, &buffer_info);
        oldcolor = buffer_info.wAttributes;
        SetConsoleTextAttribute(outfd_handle, color);
    }
    return oldcolor;
}

void end_color(HANDLE outfd_handle, WORD oldcolor)
{
    if (outfd_handle)
    {
        SetConsoleTextAttribute(outfd_handle, oldcolor);
    }
}

#define COLORED_SCOPE(handle, color, content) \
 do { \
	auto oldcolor = begin_color(handle, color); \
	content ; \
    end_color(handle, oldcolor); \
} while(false)

struct frame_content_t
{
    std::string function;
    std::string filename;
    std::size_t lineno = 0;
};

struct SymInstance final
{
    HANDLE proc;
    bool is_initialized()
    {
        return proc != NULL;
    }
    SymInstance()
    {
        proc = GetCurrentProcess();
        if (SymInitialize(proc, NULL, TRUE) != TRUE)
            proc = NULL;
    }
    ~SymInstance()
    {
        if (proc)
            SymCleanup(proc);
    }
};

// IMAGEHLP defines a variable name array that we define as NAME_BUFFER_SIZE
constexpr const int SYMINFO_SIZE = sizeof(SYMBOL_INFO) + NAME_BUFFER_SIZE;

frame_content_t parse(frame_t in_address)
{
    static SymInstance inst;
    if (!inst.is_initialized())
        return frame_content_t{};

    uint8_t syminfo_full[SYMINFO_SIZE] = { 0 };
	SYMBOL_INFO& syminfo = reinterpret_cast<SYMBOL_INFO&>(syminfo_full);
	syminfo.SizeOfStruct = sizeof(SYMBOL_INFO);
	syminfo.MaxNameLen = NAME_BUFFER_SIZE;

	if (SymFromAddr(inst.proc, (DWORD64)in_address, 0, &syminfo) == TRUE)
	{
        IMAGEHLP_LINE64 line64 = { 0 };
		line64.SizeOfStruct = sizeof(IMAGEHLP_LINE64);

		DWORD displacement = 0;
		if (SymGetLineFromAddr64(inst.proc, (DWORD64)in_address, &displacement, &line64) == TRUE)
		{
			return frame_content_t{
				syminfo.Name, // The name is already demangled
				line64.FileName,
				line64.LineNumber
			};
		}
	}

    return frame_content_t{ syminfo.Name ? syminfo.Name : std::string() };
}

inline void print_frame_content(
	std::FILE* fd,
	const frame_content_t& fc,
	const char* arrow,
    HANDLE outfd_handle = NULL
	)
{
    if (fc.filename.empty())
    {
        // Short version
        fprintf(fd, "%s ", arrow);
        COLORED_SCOPE(outfd_handle, FOREGROUND_BLUE | FOREGROUND_GREEN, {
            fprintf(fd, "%s\n", fc.function.empty() ? "<unknown>" : fc.function.c_str());
        });
    }
    else
    {
        // Long version
        fprintf(fd, "%s ", arrow);
        COLORED_SCOPE(outfd_handle, FOREGROUND_BLUE | FOREGROUND_GREEN, {
            fprintf(fd, "%s\n", fc.function.empty() ? "<unknown>" : fc.function.c_str());
        });
        fprintf(fd, "   [%s:",
            fc.filename.empty() ? "<unknown>" : fc.filename.c_str()
        );
        COLORED_SCOPE(outfd_handle, FOREGROUND_BLUE | FOREGROUND_GREEN, {
            fprintf(fd, "%zu", fc.lineno);
        });
        fprintf(fd, "]\n");
    }
}

} // namespace priv

void acquire_stacktrace(stacktrace_t& st)
{
    using namespace mst::priv;

	void* out_frames[FRAME_ALLOC_SIZE];
	std::size_t size = FRAME_ALLOC_SIZE;
	for (int i = 0; size >= FRAME_ALLOC_SIZE; ++i)
	{
		size = ::RtlCaptureStackBackTrace(i*FRAME_ALLOC_SIZE, FRAME_ALLOC_SIZE, out_frames, 0);
		std::copy(out_frames, out_frames + size, std::back_inserter(st.frames));
	}
}

void print_stacktrace(std::FILE* fd, const stacktrace_t& st)
{
    // Provide the console handle if fd is stderr or stdout
    HANDLE outfd_handle = NULL;
	if (fd == stderr)
		outfd_handle = GetStdHandle(STD_ERROR_HANDLE);
    else if (fd == stdout)
        outfd_handle = GetStdHandle(STD_OUTPUT_HANDLE);

    // We skip the first frame and the two lasts ones:
    // * 1st is acquire_stacktrace
    // * 2 last frames are before mainCRTStartup
    // FIXME: When called from single argument print_stacktrace, the function is not always inlined
    //        and ends up the callstack. Figure out whey the compiler doesn't inline even with __forceinline.
    const std::size_t size = (std::min)(
        st.frames.size(),
        st.frames.size() - 2
    );

    for (int i = 1; i < size; ++i)
    {
        using namespace mst::priv;
        print_frame_content(
           fd,
           parse(st.frames[i]),
           i == 1 ? "=>" : "^^", // Indicate the direction of the stackframe
           outfd_handle
        );
	}
}

} // namespace mst
