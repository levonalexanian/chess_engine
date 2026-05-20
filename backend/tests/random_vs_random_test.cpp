#include <chrono>
#include <cstdint>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "chess_engine/board.h"
#include "chess_engine/engines/random_mover.h"
#include "chess_engine/move.h"
#include "chess_engine/position.h"

static constexpr std::chrono::milliseconds kBudget{5};
static constexpr int kMaxFullmoves = 300;

static int king_square(chess_engine::Position const& pos, chess_engine::Color color) {
    auto const& board = pos.board();
    auto const king_piece = chess_engine::make_piece(color, chess_engine::PieceType::King);
    auto const bb = board.pieces(king_piece);
    for (int sq = 0; sq < 64; ++sq) {
        if ((bb >> sq) & 1ULL) {
            return sq;
        }
    }
    return -1;
}

static bool side_to_move_in_check(chess_engine::Position const& pos) {
    auto const stm = pos.side_to_move();
    auto const attacker = stm == chess_engine::Color::White
                              ? chess_engine::Color::Black
                              : chess_engine::Color::White;
    int const king_sq = king_square(pos, stm);
    if (king_sq < 0) {
        return false;
    }
    return pos.is_square_attacked(king_sq, attacker);
}

static void play_single_game(std::uint64_t white_seed, std::uint64_t black_seed) {
    chess_engine::Position position;
    chess_engine::RandomMoverEngine white(white_seed);
    chess_engine::RandomMoverEngine black(black_seed);

    bool terminal = false;
    for (int step = 0; step < kMaxFullmoves * 2; ++step) {
        auto const legal = position.generate_legal_moves();
        if (legal.empty()) {
            terminal = true;
            break;
        }

        auto& engine = position.white_to_move() ? white : black;
        engine.set_position(position.fen());
        auto const move = engine.best_move(kBudget);
        ASSERT_NE(move.raw(), 0u)
            << "engine returned default move while legal moves existed at "
            << position.fen();

        bool in_legal = false;
        for (auto const& m : legal) {
            if (m == move) {
                in_legal = true;
                break;
            }
        }
        ASSERT_TRUE(in_legal)
            << "engine picked illegal move " << move.to_uci()
            << " at " << position.fen();

        position.make_move(move);
        if (position.fullmove_number() > kMaxFullmoves) {
            break;
        }
    }

    if (terminal) {
        bool const in_check = side_to_move_in_check(position);
        SUCCEED() << "game terminated after " << position.fullmove_number()
                  << " fullmoves; side to move "
                  << (position.white_to_move() ? "white" : "black")
                  << (in_check ? " is in check (checkmate)" : " is stalemated");
    } else {
        SUCCEED() << "game exceeded soft fullmove bound at "
                  << position.fullmove_number();
    }
}

TEST(RandomVsRandom, GameTerminatesOrExceedsBound) {
    play_single_game(1, 2);
}

TEST(RandomVsRandom, MultipleSeededGamesAllComplete) {
    for (std::uint64_t seed = 7; seed < 12; ++seed) {
        play_single_game(seed, seed + 1000);
    }
}
