add_library(chess_engine STATIC
    src/board.cpp
    src/move.cpp
    src/move_gen.cpp
    src/perft.cpp
    src/position.cpp
    src/uci.cpp
    src/zobrist.cpp
    src/engines/random_mover.cpp
)

target_include_directories(chess_engine
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(chess_engine PUBLIC cxx_std_20)

target_link_libraries(chess_engine
    PUBLIC
        spdlog::spdlog
)

add_executable(engine-cli cli/main.cpp)
target_link_libraries(engine-cli
    PRIVATE
        chess_engine
        fmt::fmt
)

add_subdirectory(bench)
