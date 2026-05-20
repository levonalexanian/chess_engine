#include <chrono>
#include <sstream>
#include <string>
#include <string_view>
#include <variant>

#include <gtest/gtest.h>

#include "chess_engine/engine.hpp"
#include "chess_engine/move.hpp"
#include "chess_engine/uci.hpp"

namespace {

class RecordingEngine : public chess_engine::Engine {
public:
    void set_position(std::string_view fen) override {
        last_fen.assign(fen.begin(), fen.end());
        set_position_calls += 1;
    }
    chess_engine::Move best_move(std::chrono::milliseconds budget) override {
        last_budget = budget;
        best_move_calls += 1;
        return {};
    }
    std::string name() const override { return "recording"; }

    std::string last_fen;
    std::chrono::milliseconds last_budget{0};
    int set_position_calls{0};
    int best_move_calls{0};
};

}  // namespace

TEST(UciParser, ParsesUci) {
    auto cmd = chess_engine::parse_command("uci");
    ASSERT_TRUE(cmd.has_value());
    EXPECT_TRUE(std::holds_alternative<chess_engine::UciCmdUci>(*cmd));
}

TEST(UciParser, ParsesIsReady) {
    auto cmd = chess_engine::parse_command("isready");
    ASSERT_TRUE(cmd.has_value());
    EXPECT_TRUE(std::holds_alternative<chess_engine::UciCmdIsReady>(*cmd));
}

TEST(UciParser, ParsesUciNewGame) {
    auto cmd = chess_engine::parse_command("ucinewgame");
    ASSERT_TRUE(cmd.has_value());
    EXPECT_TRUE(std::holds_alternative<chess_engine::UciCmdUciNewGame>(*cmd));
}

TEST(UciParser, ParsesPositionStartpos) {
    auto cmd = chess_engine::parse_command("position startpos");
    ASSERT_TRUE(cmd.has_value());
    const auto* pos = std::get_if<chess_engine::UciCmdPosition>(&*cmd);
    ASSERT_NE(pos, nullptr);
    EXPECT_EQ(pos->fen, chess_engine::starting_fen());
    EXPECT_TRUE(pos->moves.empty());
}

TEST(UciParser, ParsesPositionStartposWithMoves) {
    auto cmd = chess_engine::parse_command("position startpos moves e2e4 e7e5");
    ASSERT_TRUE(cmd.has_value());
    const auto* pos = std::get_if<chess_engine::UciCmdPosition>(&*cmd);
    ASSERT_NE(pos, nullptr);
    EXPECT_EQ(pos->fen, chess_engine::starting_fen());
    ASSERT_EQ(pos->moves.size(), 2u);
    EXPECT_EQ(pos->moves[0], "e2e4");
    EXPECT_EQ(pos->moves[1], "e7e5");
}

TEST(UciParser, ParsesPositionFen) {
    const std::string fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    auto cmd = chess_engine::parse_command("position fen " + fen);
    ASSERT_TRUE(cmd.has_value());
    const auto* pos = std::get_if<chess_engine::UciCmdPosition>(&*cmd);
    ASSERT_NE(pos, nullptr);
    EXPECT_EQ(pos->fen, fen);
    EXPECT_TRUE(pos->moves.empty());
}

TEST(UciParser, ParsesPositionFenWithMoves) {
    const std::string fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    auto cmd = chess_engine::parse_command("position fen " + fen + " moves e2e4");
    ASSERT_TRUE(cmd.has_value());
    const auto* pos = std::get_if<chess_engine::UciCmdPosition>(&*cmd);
    ASSERT_NE(pos, nullptr);
    EXPECT_EQ(pos->fen, fen);
    ASSERT_EQ(pos->moves.size(), 1u);
    EXPECT_EQ(pos->moves[0], "e2e4");
}

TEST(UciParser, ParsesGoBare) {
    auto cmd = chess_engine::parse_command("go");
    ASSERT_TRUE(cmd.has_value());
    const auto* go = std::get_if<chess_engine::UciCmdGo>(&*cmd);
    ASSERT_NE(go, nullptr);
    EXPECT_FALSE(go->wtime.has_value());
    EXPECT_FALSE(go->btime.has_value());
    EXPECT_FALSE(go->movetime.has_value());
    EXPECT_FALSE(go->infinite);
}

TEST(UciParser, ParsesGoWithClocks) {
    auto cmd = chess_engine::parse_command("go wtime 1000 btime 1000");
    ASSERT_TRUE(cmd.has_value());
    const auto* go = std::get_if<chess_engine::UciCmdGo>(&*cmd);
    ASSERT_NE(go, nullptr);
    ASSERT_TRUE(go->wtime.has_value());
    EXPECT_EQ(*go->wtime, 1000);
    ASSERT_TRUE(go->btime.has_value());
    EXPECT_EQ(*go->btime, 1000);
}

TEST(UciParser, ParsesGoMovetime) {
    auto cmd = chess_engine::parse_command("go movetime 500");
    ASSERT_TRUE(cmd.has_value());
    const auto* go = std::get_if<chess_engine::UciCmdGo>(&*cmd);
    ASSERT_NE(go, nullptr);
    ASSERT_TRUE(go->movetime.has_value());
    EXPECT_EQ(*go->movetime, 500);
}

TEST(UciParser, ParsesGoInfiniteAndIncrements) {
    auto cmd = chess_engine::parse_command("go wtime 60000 btime 60000 winc 1000 binc 1000 infinite");
    ASSERT_TRUE(cmd.has_value());
    const auto* go = std::get_if<chess_engine::UciCmdGo>(&*cmd);
    ASSERT_NE(go, nullptr);
    EXPECT_TRUE(go->infinite);
    ASSERT_TRUE(go->winc.has_value());
    EXPECT_EQ(*go->winc, 1000);
    ASSERT_TRUE(go->binc.has_value());
    EXPECT_EQ(*go->binc, 1000);
}

TEST(UciParser, ParsesQuit) {
    auto cmd = chess_engine::parse_command("quit");
    ASSERT_TRUE(cmd.has_value());
    EXPECT_TRUE(std::holds_alternative<chess_engine::UciCmdQuit>(*cmd));
}

TEST(UciParser, ParsesDShortAndLong) {
    auto d_cmd = chess_engine::parse_command("d");
    ASSERT_TRUE(d_cmd.has_value());
    EXPECT_TRUE(std::holds_alternative<chess_engine::UciCmdDisplay>(*d_cmd));

    auto display_cmd = chess_engine::parse_command("display");
    ASSERT_TRUE(display_cmd.has_value());
    EXPECT_TRUE(std::holds_alternative<chess_engine::UciCmdDisplay>(*display_cmd));
}

TEST(UciParser, IgnoresUnknownCommands) {
    EXPECT_FALSE(chess_engine::parse_command("setoption name Hash value 16").has_value());
    EXPECT_FALSE(chess_engine::parse_command("debug on").has_value());
    EXPECT_FALSE(chess_engine::parse_command("").has_value());
    EXPECT_FALSE(chess_engine::parse_command("   ").has_value());
}

TEST(UciParser, ToleratesExtraWhitespace) {
    auto cmd = chess_engine::parse_command("   position   startpos   moves   e2e4   ");
    ASSERT_TRUE(cmd.has_value());
    const auto* pos = std::get_if<chess_engine::UciCmdPosition>(&*cmd);
    ASSERT_NE(pos, nullptr);
    EXPECT_EQ(pos->fen, chess_engine::starting_fen());
    ASSERT_EQ(pos->moves.size(), 1u);
    EXPECT_EQ(pos->moves[0], "e2e4");
}

TEST(UciLoopIntegration, HandshakeAndBestMove) {
    RecordingEngine engine;
    std::istringstream in(
        "uci\n"
        "isready\n"
        "ucinewgame\n"
        "position startpos moves e2e4\n"
        "go movetime 250\n"
        "quit\n");
    std::ostringstream out;
    chess_engine::UciLoop::run(engine, in, out);
    const std::string output = out.str();
    EXPECT_NE(output.find("id name "), std::string::npos);
    EXPECT_NE(output.find("id author "), std::string::npos);
    EXPECT_NE(output.find("uciok"), std::string::npos);
    EXPECT_NE(output.find("readyok"), std::string::npos);
    EXPECT_NE(output.find("bestmove 0000"), std::string::npos);
    EXPECT_GE(engine.set_position_calls, 2);
    // The engine sees the position after the move list is applied, not the
    // base FEN, so it can pick a move based on the current state.
    EXPECT_EQ(engine.last_fen,
              "rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1");
    EXPECT_EQ(engine.best_move_calls, 1);
    EXPECT_EQ(engine.last_budget, std::chrono::milliseconds{250});
}

TEST(UciLoopIntegration, QuitStopsLoop) {
    RecordingEngine engine;
    std::istringstream in("quit\nuci\n");
    std::ostringstream out;
    chess_engine::UciLoop::run(engine, in, out);
    EXPECT_TRUE(out.str().empty());
}

TEST(UciLoopIntegration, IgnoresUnknownLines) {
    RecordingEngine engine;
    std::istringstream in(
        "debug on\n"
        "setoption name Hash value 16\n"
        "isready\n"
        "quit\n");
    std::ostringstream out;
    chess_engine::UciLoop::run(engine, in, out);
    EXPECT_EQ(out.str(), "readyok\n");
}

TEST(UciLoopIntegration, DisplayRendersStartingPosition) {
    RecordingEngine engine;
    std::istringstream in(
        "position startpos\n"
        "d\n"
        "quit\n");
    std::ostringstream out;
    chess_engine::UciLoop::run(engine, in, out);
    const std::string output = out.str();
    EXPECT_NE(output.find("r n b q k b n r"), std::string::npos);
    EXPECT_NE(output.find("P P P P P P P P"), std::string::npos);
    EXPECT_NE(output.find("a b c d e f g h"), std::string::npos);
    EXPECT_NE(output.find("Side to move: white"), std::string::npos);
    EXPECT_NE(output.find("Castling: KQkq"), std::string::npos);
    EXPECT_NE(output.find("En passant: -"), std::string::npos);
    EXPECT_NE(output.find("Halfmove clock: 0"), std::string::npos);
    EXPECT_NE(output.find("Fullmove number: 1"), std::string::npos);
    EXPECT_NE(output.find("Zobrist hash: 0x"), std::string::npos);
}

TEST(UciLoopIntegration, DisplayReflectsAppliedMoves) {
    RecordingEngine engine;
    std::istringstream in(
        "position startpos moves e2e4 e7e5\n"
        "d\n"
        "quit\n");
    std::ostringstream out;
    chess_engine::UciLoop::run(engine, in, out);
    const std::string output = out.str();
    // After 1. e4 e5, e2 and e7 are empty (replaced with dots), and e4/e5 hold pawns.
    EXPECT_NE(output.find("p p p p . p p p"), std::string::npos);  // black pawn row (rank 7)
    EXPECT_NE(output.find("P P P P . P P P"), std::string::npos);  // white pawn row (rank 2)
    EXPECT_NE(output.find("Side to move: white"), std::string::npos);
    EXPECT_NE(output.find("Fullmove number: 2"), std::string::npos);
}

TEST(UciLoopIntegration, DisplayReflectsFenPosition) {
    RecordingEngine engine;
    std::istringstream in(
        "position fen 8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1\n"
        "d\n"
        "quit\n");
    std::ostringstream out;
    chess_engine::UciLoop::run(engine, in, out);
    const std::string output = out.str();
    EXPECT_NE(output.find("Castling: -"), std::string::npos);
    EXPECT_NE(output.find("K P . . . . . r"), std::string::npos);  // rank 5
}
