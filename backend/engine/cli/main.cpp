#include <iostream>
#include <memory>

#include "chess_engine/engine.h"
#include "chess_engine/engines/random_mover.h"
#include "chess_engine/uci.h"

int main() {
    std::unique_ptr<chess_engine::Engine> engine = std::make_unique<chess_engine::RandomMoverEngine>();
    chess_engine::UciLoop::run(*engine, std::cin, std::cout);
    return 0;
}
