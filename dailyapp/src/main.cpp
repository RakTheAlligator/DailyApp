#include <iostream>
#include <string_view>
#include <vector>
#include <span>

#include "WeightCli.hpp"
#include "FoodCli.hpp"

static void print_help() {
    std::cout <<
R"(DailyApp launcher

Usage:
  DailyApp --help
  DailyApp <tracker> --help
  DailyApp weight <command> [args...]
  DailyApp food   <command> [args...]

Trackers:
  weight   Weight tracker
  food     Food tracker
)";
}

int main(int argc, char** argv) {
    std::vector<std::string_view> args;
    args.reserve(static_cast<size_t>(argc));
    for (int i = 0; i < argc; ++i) args.emplace_back(argv[i]);

    if (args.size() <= 1 || args[1] == "--help" || args[1] == "-h") {
        print_help();
        return 0;
    }

    const std::string_view tracker = args[1];

    // subArgs = everything after "<tracker>"
    std::span<const std::string_view> subArgs(args.data() + 2, args.size() - 2);

    if (tracker == "weight") {
        // Allow: DailyApp weight --help
        return weight::run(subArgs);
    }
    if (tracker == "food") {
        return food::run(subArgs);
    }

    std::cerr << "Unknown tracker: " << tracker << "\n\n";
    print_help();
    return 2;
}
