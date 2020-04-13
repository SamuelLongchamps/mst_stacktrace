# MST - Modest Stacktrace

Modest header-only stacktrace printer for C++
> ### modest `[mod-ist]`
> having or showing a moderate or humble estimate of one's merits, importance, etc.; free from vanity, egotism, boastfulness, or great pretensions

Usage:
```cpp
#include "mst/stacktrace.h"

void fct() {
    mst::print_stacktrace(stderr);
}
```

* Supports Linux, macOS and Windows. Tested with clang-8 and MSVC v142.
* Output is colored and uniform across all supported platforms.
* The path is absolute, for automated link detection (vscode)

![mst showoff](https://user-images.githubusercontent.com/5256911/79162170-cb7b7700-7daa-11ea-8f16-8ab5420c7c30.png)

## Requirements
* On Linux and macOS, linking to libbacktrace and libdl is required.
* On Windows, pragma comments automatically add Dbghelp and kernel32 as linking dependencies.

To get file and line information information, the binary where the function resides has to be compiled with debug information. (e.g., `-g` flag for gcc and clang)

In the absence of debug information, only the function names will be printed.

## Related work

* [Backward](https://github.com/bombela/backward-cpp)
* [Boost Stacktrace](https://github.com/boostorg/stacktrace)
* [Chromium Stacktrace](https://github.com/chromium/chromium/tree/master/base/debug)
