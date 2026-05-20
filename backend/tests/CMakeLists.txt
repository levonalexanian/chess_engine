find_package(GTest CONFIG REQUIRED)

include(GoogleTest)

add_executable(backend_smoke_tests smoke_test.cpp)
target_link_libraries(backend_smoke_tests
    PRIVATE
        GTest::gtest
        GTest::gtest_main
)

gtest_discover_tests(backend_smoke_tests)

add_executable(engine_tests engine_test.cpp)
target_link_libraries(engine_tests
    PRIVATE
        chess_engine
        GTest::gtest
        GTest::gtest_main
)

gtest_discover_tests(engine_tests)

add_executable(board_tests board_test.cpp)
target_link_libraries(board_tests
    PRIVATE
        chess_engine
        GTest::gtest
        GTest::gtest_main
)

gtest_discover_tests(board_tests)

add_executable(move_tests move_test.cpp)
target_link_libraries(move_tests
    PRIVATE
        chess_engine
        GTest::gtest
        GTest::gtest_main
)

gtest_discover_tests(move_tests)

add_executable(position_tests position_test.cpp)
target_link_libraries(position_tests
    PRIVATE
        chess_engine
        GTest::gtest
        GTest::gtest_main
)

gtest_discover_tests(position_tests)

add_executable(zobrist_tests zobrist_test.cpp)
target_link_libraries(zobrist_tests
    PRIVATE
        chess_engine
        GTest::gtest
        GTest::gtest_main
)

gtest_discover_tests(zobrist_tests)

add_executable(make_move_tests make_move_test.cpp)
target_link_libraries(make_move_tests
    PRIVATE
        chess_engine
        GTest::gtest
        GTest::gtest_main
)

gtest_discover_tests(make_move_tests)

add_executable(attacks_tests attacks_test.cpp)
target_link_libraries(attacks_tests
    PRIVATE
        chess_engine
        GTest::gtest
        GTest::gtest_main
)

gtest_discover_tests(attacks_tests)

add_executable(move_gen_tests move_gen_test.cpp)
target_link_libraries(move_gen_tests
    PRIVATE
        chess_engine
        GTest::gtest
        GTest::gtest_main
)

gtest_discover_tests(move_gen_tests)

add_executable(unmake_tests unmake_test.cpp)
target_link_libraries(unmake_tests
    PRIVATE
        chess_engine
        GTest::gtest
        GTest::gtest_main
)

gtest_discover_tests(unmake_tests)

add_executable(legal_moves_tests legal_moves_test.cpp)
target_link_libraries(legal_moves_tests
    PRIVATE
        chess_engine
        GTest::gtest
        GTest::gtest_main
)

gtest_discover_tests(legal_moves_tests)

add_executable(uci_tests uci_test.cpp)
target_link_libraries(uci_tests
    PRIVATE
        chess_engine
        GTest::gtest
        GTest::gtest_main
)

gtest_discover_tests(uci_tests)

add_executable(server_tests server_test.cpp)
target_link_libraries(server_tests
    PRIVATE
        chess_server_lib
        GTest::gtest
        GTest::gtest_main
)

gtest_discover_tests(server_tests)

add_executable(game_session_tests game_session_test.cpp)
target_link_libraries(game_session_tests
    PRIVATE
        chess_server_lib
        chess_engine
        GTest::gtest
        GTest::gtest_main
)

gtest_discover_tests(game_session_tests)

add_executable(perft_tests perft_test.cpp)
target_link_libraries(perft_tests
    PRIVATE
        chess_engine
        GTest::gtest
        GTest::gtest_main
)

gtest_discover_tests(perft_tests)

add_executable(random_mover_tests random_mover_test.cpp)
target_link_libraries(random_mover_tests
    PRIVATE
        chess_engine
        GTest::gtest
        GTest::gtest_main
)

gtest_discover_tests(random_mover_tests)

add_executable(random_vs_random_tests random_vs_random_test.cpp)
target_link_libraries(random_vs_random_tests
    PRIVATE
        chess_engine
        GTest::gtest
        GTest::gtest_main
)

gtest_discover_tests(random_vs_random_tests)
