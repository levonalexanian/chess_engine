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
