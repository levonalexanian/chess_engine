add_executable(perft-bench perft_bench.cpp)
target_link_libraries(perft-bench
    PRIVATE
        chess_engine
        fmt::fmt
)

set_target_properties(perft-bench PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)
