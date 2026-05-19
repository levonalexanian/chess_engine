find_package(GTest CONFIG REQUIRED)

include(GoogleTest)

add_executable(backend_smoke_tests smoke_test.cpp)
target_link_libraries(backend_smoke_tests
    PRIVATE
        GTest::gtest
        GTest::gtest_main
)

gtest_discover_tests(backend_smoke_tests)
