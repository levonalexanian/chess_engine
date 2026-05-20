#include <memory>
#include <string>
#include <vector>

#include <crow/json.h>
#include <gtest/gtest.h>

#include "chess_server/app.h"
#include "chess_server/game_session.h"
#include "chess_server/registry.h"

namespace {

constexpr char const* kStartingFen =
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

}  // namespace

TEST(EngineRegistry, DefaultsRegisterPlaceholderAndRandom) {
    auto registry = chess_server::EngineRegistry::with_defaults();
    EXPECT_TRUE(registry.has("placeholder"));
    EXPECT_TRUE(registry.has("random"));
    EXPECT_FALSE(registry.has("nonexistent"));

    auto placeholder = registry.create("placeholder");
    ASSERT_NE(placeholder, nullptr);
    EXPECT_EQ(placeholder->name(), "placeholder");

    auto random = registry.create("random");
    ASSERT_NE(random, nullptr);
    EXPECT_EQ(random->name(), "random");

    EXPECT_EQ(registry.create("nonexistent"), nullptr);
}

TEST(EngineRegistry, DefaultEngineNameIsRandom) {
    EXPECT_EQ(chess_server::default_engine_name(), "random");
}

TEST(GameSession, NewGameTransitionsToInGameAndEmitsState) {
    auto registry = chess_server::EngineRegistry::with_defaults();
    chess_server::GameSession session(registry);
    EXPECT_EQ(session.state(), chess_server::SessionState::Idle);

    auto out = session.on_new_game("placeholder");

    ASSERT_EQ(out.size(), 1u);
    EXPECT_EQ(out[0].type, "state");
    EXPECT_EQ(out[0].fen, kStartingFen);
    EXPECT_EQ(session.state(), chess_server::SessionState::InGame);
    ASSERT_NE(session.engine(), nullptr);
    EXPECT_EQ(session.engine()->name(), "placeholder");
}

TEST(GameSession, NewGameWithUnknownEngineReturnsError) {
    auto registry = chess_server::EngineRegistry::with_defaults();
    chess_server::GameSession session(registry);

    auto out = session.on_new_game("missing-engine");

    ASSERT_EQ(out.size(), 1u);
    EXPECT_EQ(out[0].type, "error");
    EXPECT_NE(out[0].message.find("missing-engine"), std::string::npos);
    EXPECT_EQ(session.state(), chess_server::SessionState::Idle);
}

TEST(GameSession, UserMoveBeforeNewGameIsError) {
    auto registry = chess_server::EngineRegistry::with_defaults();
    chess_server::GameSession session(registry);

    auto out = session.on_user_move("e2e4");

    ASSERT_EQ(out.size(), 1u);
    EXPECT_EQ(out[0].type, "error");
}

TEST(GameSession, UserMoveAppliesToPositionAndAcknowledges) {
    auto registry = chess_server::EngineRegistry::with_defaults();
    chess_server::GameSession session(registry);
    session.on_new_game("placeholder");

    auto out = session.on_user_move("e2e4");

    ASSERT_EQ(out.size(), 1u);
    EXPECT_EQ(out[0].type, "state");
    EXPECT_EQ(out[0].fen,
              "rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1");
    EXPECT_EQ(session.current_fen(),
              "rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1");
    ASSERT_TRUE(session.last_user_move().has_value());
    EXPECT_EQ(*session.last_user_move(), "e2e4");
}

TEST(GameSession, UserMoveRejectsIllegalMove) {
    auto registry = chess_server::EngineRegistry::with_defaults();
    chess_server::GameSession session(registry);
    session.on_new_game("placeholder");

    auto out = session.on_user_move("e2e5");

    ASSERT_EQ(out.size(), 1u);
    EXPECT_EQ(out[0].type, "error");
    EXPECT_NE(out[0].message.find("illegal"), std::string::npos);
    EXPECT_EQ(session.current_fen(), kStartingFen);
}

TEST(GameSession, UserMoveRejectsMalformedUci) {
    auto registry = chess_server::EngineRegistry::with_defaults();
    chess_server::GameSession session(registry);
    session.on_new_game("placeholder");

    auto out = session.on_user_move("zz99");

    ASSERT_EQ(out.size(), 1u);
    EXPECT_EQ(out[0].type, "error");
    EXPECT_NE(out[0].message.find("malformed"), std::string::npos);
    EXPECT_EQ(session.current_fen(), kStartingFen);
}

TEST(GameSession, RequestEngineMoveBeforeNewGameIsError) {
    auto registry = chess_server::EngineRegistry::with_defaults();
    chess_server::GameSession session(registry);

    auto out = session.on_request_engine_move();

    ASSERT_EQ(out.size(), 1u);
    EXPECT_EQ(out[0].type, "error");
}

TEST(GameSession, RequestEngineMoveReturnsEngineMoveAndState) {
    auto registry = chess_server::EngineRegistry::with_defaults();
    chess_server::GameSession session(registry);
    session.on_new_game("placeholder");
    session.on_user_move("e2e4");

    auto out = session.on_request_engine_move();

    ASSERT_EQ(out.size(), 2u);
    EXPECT_EQ(out[0].type, "engine_move");
    EXPECT_EQ(out[0].uci, "0000");
    EXPECT_EQ(out[1].type, "state");
    // Placeholder engine returns the null move, so the position only reflects
    // the user's e2e4 push.
    EXPECT_EQ(out[1].fen,
              "rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1");
}

TEST(GameSession, RequestEngineMoveAppliesRandomEngineReply) {
    auto registry = chess_server::EngineRegistry::with_defaults();
    chess_server::GameSession session(registry);
    session.on_new_game("random");
    auto user_out = session.on_user_move("e2e4");
    ASSERT_EQ(user_out.size(), 1u);
    auto const after_user = session.current_fen();

    auto out = session.on_request_engine_move();

    ASSERT_EQ(out.size(), 2u);
    EXPECT_EQ(out[0].type, "engine_move");
    EXPECT_NE(out[0].uci, "0000");
    EXPECT_EQ(out[1].type, "state");
    EXPECT_NE(out[1].fen, after_user);
    EXPECT_EQ(out[1].fen, session.current_fen());
}

TEST(GameSession, SerializeStateMessageProducesValidJson) {
    auto msg = chess_server::make_state_message(kStartingFen);
    auto raw = chess_server::serialize_outbound(msg);

    auto parsed = crow::json::load(raw);
    ASSERT_TRUE(parsed);
    EXPECT_EQ(std::string(parsed["type"].s()), "state");
    EXPECT_EQ(std::string(parsed["fen"].s()), kStartingFen);
}

TEST(GameSession, SerializeEngineMoveMessageProducesValidJson) {
    auto msg = chess_server::make_engine_move_message("0000");
    auto raw = chess_server::serialize_outbound(msg);

    auto parsed = crow::json::load(raw);
    ASSERT_TRUE(parsed);
    EXPECT_EQ(std::string(parsed["type"].s()), "engine_move");
    EXPECT_EQ(std::string(parsed["uci"].s()), "0000");
}

TEST(GameSession, SerializeErrorMessageProducesValidJson) {
    auto msg = chess_server::make_error_message("bad json");
    auto raw = chess_server::serialize_outbound(msg);

    auto parsed = crow::json::load(raw);
    ASSERT_TRUE(parsed);
    EXPECT_EQ(std::string(parsed["type"].s()), "error");
    EXPECT_EQ(std::string(parsed["message"].s()), "bad json");
}

TEST(AppDispatch, BadJsonProducesError) {
    auto registry = chess_server::EngineRegistry::with_defaults();
    chess_server::GameSession session(registry);

    auto out = chess_server::App::dispatch_message(session, "not json");

    ASSERT_EQ(out.size(), 1u);
    EXPECT_EQ(out[0].type, "error");
    EXPECT_EQ(out[0].message, "bad json");
}

TEST(AppDispatch, MissingTypeProducesError) {
    auto registry = chess_server::EngineRegistry::with_defaults();
    chess_server::GameSession session(registry);

    auto out = chess_server::App::dispatch_message(session, R"({"foo":"bar"})");

    ASSERT_EQ(out.size(), 1u);
    EXPECT_EQ(out[0].type, "error");
}

TEST(AppDispatch, UnknownTypeProducesError) {
    auto registry = chess_server::EngineRegistry::with_defaults();
    chess_server::GameSession session(registry);

    auto out = chess_server::App::dispatch_message(session, R"({"type":"flarp"})");

    ASSERT_EQ(out.size(), 1u);
    EXPECT_EQ(out[0].type, "error");
    EXPECT_NE(out[0].message.find("flarp"), std::string::npos);
}

TEST(AppDispatch, FullHappyPath) {
    auto registry = chess_server::EngineRegistry::with_defaults();
    chess_server::GameSession session(registry);

    auto new_game_out = chess_server::App::dispatch_message(
        session, R"({"type":"new_game","engine":"placeholder"})");
    ASSERT_EQ(new_game_out.size(), 1u);
    EXPECT_EQ(new_game_out[0].type, "state");

    auto user_move_out = chess_server::App::dispatch_message(
        session, R"({"type":"user_move","uci":"e2e4"})");
    ASSERT_EQ(user_move_out.size(), 1u);
    EXPECT_EQ(user_move_out[0].type, "state");

    auto engine_move_out = chess_server::App::dispatch_message(
        session, R"({"type":"request_engine_move"})");
    ASSERT_EQ(engine_move_out.size(), 2u);
    EXPECT_EQ(engine_move_out[0].type, "engine_move");
    EXPECT_EQ(engine_move_out[0].uci, "0000");
    EXPECT_EQ(engine_move_out[1].type, "state");
    EXPECT_EQ(engine_move_out[1].fen,
              "rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1");
}

TEST(AppDispatch, NewGameWithoutEngineFieldUsesDefault) {
    auto registry = chess_server::EngineRegistry::with_defaults();
    chess_server::GameSession session(registry);

    auto out = chess_server::App::dispatch_message(session, R"({"type":"new_game"})");

    ASSERT_EQ(out.size(), 1u);
    EXPECT_EQ(out[0].type, "state");
    EXPECT_EQ(session.state(), chess_server::SessionState::InGame);
    ASSERT_NE(session.engine(), nullptr);
    EXPECT_EQ(session.engine()->name(), std::string(chess_server::default_engine_name()));
}

TEST(AppDispatch, NewGameWithEmptyEngineFieldUsesDefault) {
    auto registry = chess_server::EngineRegistry::with_defaults();
    chess_server::GameSession session(registry);

    auto out = chess_server::App::dispatch_message(session, R"({"type":"new_game","engine":""})");

    ASSERT_EQ(out.size(), 1u);
    EXPECT_EQ(out[0].type, "state");
    ASSERT_NE(session.engine(), nullptr);
    EXPECT_EQ(session.engine()->name(), std::string(chess_server::default_engine_name()));
}

TEST(GameSession, NewGameWithStartingFenInitializesPosition) {
    auto registry = chess_server::EngineRegistry::with_defaults();
    chess_server::GameSession session(registry);
    std::string const after_e4 =
        "rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1";

    auto out = session.on_new_game("placeholder", after_e4, {});

    ASSERT_EQ(out.size(), 1u);
    EXPECT_EQ(out[0].type, "state");
    EXPECT_EQ(out[0].fen, after_e4);
    EXPECT_EQ(session.current_fen(), after_e4);
    EXPECT_EQ(session.state(), chess_server::SessionState::InGame);
}

TEST(GameSession, NewGameWithMovesReplaysThem) {
    auto registry = chess_server::EngineRegistry::with_defaults();
    chess_server::GameSession session(registry);

    auto out = session.on_new_game(
        "placeholder", std::nullopt,
        std::vector<std::string>{"e2e4", "e7e5", "g1f3", "b8c6", "f1c4"});

    ASSERT_EQ(out.size(), 1u);
    EXPECT_EQ(out[0].type, "state");
    EXPECT_EQ(session.current_fen(),
              "r1bqkbnr/pppp1ppp/2n5/4p3/2B1P3/5N2/PPPP1PPP/RNBQK2R b KQkq - 3 3");
    ASSERT_TRUE(session.last_user_move().has_value());
    EXPECT_EQ(*session.last_user_move(), "f1c4");
}

TEST(GameSession, NewGameRejectsMalformedStartingFen) {
    auto registry = chess_server::EngineRegistry::with_defaults();
    chess_server::GameSession session(registry);

    auto out = session.on_new_game("placeholder", std::string{"not a fen"}, {});

    ASSERT_EQ(out.size(), 1u);
    EXPECT_EQ(out[0].type, "error");
    EXPECT_NE(out[0].message.find("starting_fen"), std::string::npos);
    EXPECT_EQ(session.state(), chess_server::SessionState::Idle);
}

TEST(GameSession, NewGameRejectsIllegalMoveHistory) {
    auto registry = chess_server::EngineRegistry::with_defaults();
    chess_server::GameSession session(registry);

    auto out = session.on_new_game(
        "placeholder", std::nullopt,
        std::vector<std::string>{"e2e4", "e2e4"});

    ASSERT_EQ(out.size(), 1u);
    EXPECT_EQ(out[0].type, "error");
    EXPECT_NE(out[0].message.find("illegal"), std::string::npos);
    EXPECT_EQ(session.state(), chess_server::SessionState::Idle);
}

TEST(GameSession, NewGameWithStartingFenAndMovesCombined) {
    auto registry = chess_server::EngineRegistry::with_defaults();
    chess_server::GameSession session(registry);
    std::string const after_e4 =
        "rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1";

    auto out = session.on_new_game(
        "placeholder", after_e4,
        std::vector<std::string>{"e7e5", "g1f3"});

    ASSERT_EQ(out.size(), 1u);
    EXPECT_EQ(out[0].type, "state");
    EXPECT_EQ(session.current_fen(),
              "rnbqkbnr/pppp1ppp/8/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R b KQkq - 1 2");
}

TEST(AppDispatch, NewGameAcceptsStartingFenAndMovesViaJson) {
    auto registry = chess_server::EngineRegistry::with_defaults();
    chess_server::GameSession session(registry);

    auto out = chess_server::App::dispatch_message(
        session,
        R"({"type":"new_game","engine":"placeholder","moves":["e2e4","e7e5"]})");

    ASSERT_EQ(out.size(), 1u);
    EXPECT_EQ(out[0].type, "state");
    EXPECT_EQ(out[0].fen,
              "rnbqkbnr/pppp1ppp/8/4p3/4P3/8/PPPP1PPP/RNBQKBNR w KQkq e6 0 2");
}

TEST(AppDispatch, NewGameRejectsMovesArrayWithNonStringEntry) {
    auto registry = chess_server::EngineRegistry::with_defaults();
    chess_server::GameSession session(registry);

    auto out = chess_server::App::dispatch_message(
        session,
        R"({"type":"new_game","engine":"placeholder","moves":["e2e4",42]})");

    ASSERT_EQ(out.size(), 1u);
    EXPECT_EQ(out[0].type, "error");
}

TEST(AppDispatch, NewGameRejectsStartingFenWrongType) {
    auto registry = chess_server::EngineRegistry::with_defaults();
    chess_server::GameSession session(registry);

    auto out = chess_server::App::dispatch_message(
        session,
        R"({"type":"new_game","engine":"placeholder","starting_fen":42})");

    ASSERT_EQ(out.size(), 1u);
    EXPECT_EQ(out[0].type, "error");
}

TEST(AppDispatch, UserMoveWithoutUciFieldProducesError) {
    auto registry = chess_server::EngineRegistry::with_defaults();
    chess_server::GameSession session(registry);
    session.on_new_game("placeholder");

    auto out = chess_server::App::dispatch_message(session, R"({"type":"user_move"})");

    ASSERT_EQ(out.size(), 1u);
    EXPECT_EQ(out[0].type, "error");
}
