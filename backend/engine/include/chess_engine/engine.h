#ifndef CHESS_ENGINE_ENGINE_H
#define CHESS_ENGINE_ENGINE_H

#include <chrono>
#include <string>
#include <string_view>

#include "chess_engine/move.h"

namespace chess_engine {

// Abstract Engine interface — the seam every concrete engine plugs into
// (RandomMover, NnPolicy, NnMcts, ExternalUci, ...). The server and engine-cli
// both talk to engines exclusively through this interface.
class Engine {
public:
    virtual ~Engine() = default;

    virtual void set_position(std::string_view fen) = 0;
    virtual Move best_move(std::chrono::milliseconds time_budget) = 0;
    virtual std::string name() const = 0;
};

}  // namespace chess_engine

#endif  // CHESS_ENGINE_ENGINE_H
