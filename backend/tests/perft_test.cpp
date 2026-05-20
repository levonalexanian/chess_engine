#include <cstdlib>
#include <string>

#include <gtest/gtest.h>

#include "chess_engine/perft.h"
#include "chess_engine/position.h"

namespace ce = chess_engine;

namespace {

ce::Position pos_from(const char* fen) {
    auto p = ce::Position::from_fen(fen);
    [&]() { ASSERT_TRUE(p.has_value()) << fen; }();
    return *p;
}

}  // namespace

TEST(PerftDepth4, Startpos) {
    auto pos = pos_from("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    EXPECT_EQ(ce::perft(pos, 4), 197281ULL);
}

TEST(PerftDepth4, Kiwipete) {
    auto pos = pos_from("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1");
    EXPECT_EQ(ce::perft(pos, 4), 4085603ULL);
}

TEST(PerftDepth4, Pos3) {
    auto pos = pos_from("8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1");
    EXPECT_EQ(ce::perft(pos, 4), 43238ULL);
}

TEST(PerftDepth4, Pos4) {
    auto pos = pos_from("r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1");
    EXPECT_EQ(ce::perft(pos, 4), 422333ULL);
}

TEST(PerftDepth4, Pos5) {
    auto pos = pos_from("rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8");
    EXPECT_EQ(ce::perft(pos, 4), 2103487ULL);
}

TEST(PerftDepth4, Pos6) {
    auto pos = pos_from("r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10");
    EXPECT_EQ(ce::perft(pos, 4), 3894594ULL);
}

TEST(PerftDepth5, Startpos) {
    if (!std::getenv("CHESS_ENGINE_PERFT_DEEP")) GTEST_SKIP();
    auto pos = pos_from("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    EXPECT_EQ(ce::perft(pos, 5), 4865609ULL);
}

TEST(PerftDepth5, Kiwipete) {
    if (!std::getenv("CHESS_ENGINE_PERFT_DEEP")) GTEST_SKIP();
    auto pos = pos_from("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1");
    EXPECT_EQ(ce::perft(pos, 5), 193690690ULL);
}

TEST(PerftDepth5, Pos3) {
    if (!std::getenv("CHESS_ENGINE_PERFT_DEEP")) GTEST_SKIP();
    auto pos = pos_from("8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1");
    EXPECT_EQ(ce::perft(pos, 5), 674624ULL);
}

TEST(PerftDepth5, Pos4) {
    if (!std::getenv("CHESS_ENGINE_PERFT_DEEP")) GTEST_SKIP();
    auto pos = pos_from("r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1");
    EXPECT_EQ(ce::perft(pos, 5), 15833292ULL);
}

TEST(PerftDepth5, Pos5) {
    if (!std::getenv("CHESS_ENGINE_PERFT_DEEP")) GTEST_SKIP();
    auto pos = pos_from("rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8");
    EXPECT_EQ(ce::perft(pos, 5), 89941194ULL);
}

TEST(PerftDepth5, Pos6) {
    if (!std::getenv("CHESS_ENGINE_PERFT_DEEP")) GTEST_SKIP();
    auto pos = pos_from("r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10");
    EXPECT_EQ(ce::perft(pos, 5), 164075551ULL);
}
