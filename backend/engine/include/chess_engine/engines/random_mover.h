#ifndef CHESS_ENGINE_ENGINES_RANDOM_MOVER_H
#define CHESS_ENGINE_ENGINES_RANDOM_MOVER_H

#include <chrono>
#include <cstdint>
#include <random>
#include <string>
#include <string_view>

#include "chess_engine/engine.hpp"
#include "chess_engine/move.hpp"
#include "chess_engine/position.hpp"

namespace chess_engine {

class RandomMoverEngine : public Engine {
public:
    RandomMoverEngine();
    explicit RandomMoverEngine(std::uint64_t seed);

    void set_position(std::string_view fen) override;
    Move best_move(std::chrono::milliseconds time_budget) override;
    std::string name() const override;

private:
    Position position_;
    std::mt19937_64 rng_;
};

}  // namespace chess_engine

#endif  // CHESS_ENGINE_ENGINES_RANDOM_MOVER_H
