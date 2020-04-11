#pragma once

namespace mst::priv {
    namespace console {
        constexpr const char* red = "\e[0;31m";
        constexpr const char* RED = "\e[1;31m";
        constexpr const char* blue = "\e[0;34m";
        constexpr const char* BLUE = "\e[1;34m";
        constexpr const char* cyan = "\e[0;36m";
        constexpr const char* CYAN = "\e[1;36m";
        constexpr const char* green = "\e[0;32m";
        constexpr const char* GREEN = "\e[1;32m";
        constexpr const char* yellow = "\e[0;33m";
        constexpr const char* YELLOW = "\e[1;33m";
        constexpr const char* NC = "\e[0m";
    }

    constexpr const int FRAME_ALLOC_SIZE = 128;
}