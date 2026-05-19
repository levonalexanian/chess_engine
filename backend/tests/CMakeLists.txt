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
