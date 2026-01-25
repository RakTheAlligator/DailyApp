#pragma once
#include <span>
#include <string_view>

namespace food {
    int run(std::span<const std::string_view> args);
}
