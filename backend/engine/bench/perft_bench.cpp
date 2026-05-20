#include <chrono>
#include <cstdint>
#include <string>

#include <fmt/format.h>

#include "chess_engine/perft.h"
#include "chess_engine/position.h"

namespace ce = chess_engine;

int main(int argc, char* argv[]) {
    const std::string fen =
        (argc >= 2) ? argv[1]
                    : "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    const int depth = (argc >= 3) ? std::stoi(argv[2]) : 5;

    auto pos_opt = ce::Position::from_fen(fen);
    if (!pos_opt) {
        fmt::print("Error: invalid FEN\n");
        return 1;
    }
    auto& pos = *pos_opt;

    const auto t0 = std::chrono::steady_clock::now();
    const std::uint64_t nodes = ce::perft(pos, depth);
    const auto t1 = std::chrono::steady_clock::now();

    const double elapsed_ms =
        std::chrono::duration<double, std::milli>(t1 - t0).count();
    const double elapsed_s = elapsed_ms / 1000.0;
    const double mnps = (elapsed_s > 0) ? (static_cast<double>(nodes) / elapsed_s / 1e6) : 0;

    fmt::print("FEN:   {}\n", fen);
    fmt::print("Depth: {}\n", depth);
    fmt::print("Nodes: {}\n", nodes);
    fmt::print("Time:  {:.0f} ms\n", elapsed_ms);
    fmt::print("Speed: {:.2f} Mn/s\n", mnps);

    return 0;
}
