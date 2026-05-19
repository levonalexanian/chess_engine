#pragma once

#include <iosfwd>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace chess_engine {

class Engine;

struct UciCmdUci {};
struct UciCmdIsReady {};
struct UciCmdUciNewGame {};
struct UciCmdQuit {};

struct UciCmdPosition {
    std::string fen;
    std::vector<std::string> moves;
};

struct UciCmdGo {
    std::optional<int> wtime;
    std::optional<int> btime;
    std::optional<int> winc;
    std::optional<int> binc;
    std::optional<int> movetime;
    std::optional<int> depth;
    std::optional<int> nodes;
    bool infinite{false};
};

using UciCommand = std::variant<
    UciCmdUci,
    UciCmdIsReady,
    UciCmdUciNewGame,
    UciCmdPosition,
    UciCmdGo,
    UciCmdQuit>;

std::optional<UciCommand> parse_command(std::string_view line);

std::string starting_fen();

class UciLoop {
public:
    static void run(Engine& engine, std::istream& in, std::ostream& out);
};

}  // namespace chess_engine
