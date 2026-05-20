#include <iostream>
#include <memory>

#include "chess_engine/engine.hpp"
#include "chess_engine/engines/random_mover.hpp"
#include "chess_engine/uci.hpp"

int main() {
    std::unique_ptr<chess_engine::Engine> engine = std::make_unique<chess_engine::RandomMoverEngine>();
    chess_engine::UciLoop::run(*engine, std::cin, std::cout);
    return 0;
}
