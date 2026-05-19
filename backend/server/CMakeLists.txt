find_package(Crow CONFIG REQUIRED)

add_library(chess_server_lib STATIC
    src/app.cpp
    src/game_session.cpp
    src/registry.cpp
)

target_include_directories(chess_server_lib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(chess_server_lib
    PUBLIC
        Crow::Crow
        spdlog::spdlog
        fmt::fmt
        chess_engine
)

target_compile_features(chess_server_lib PUBLIC cxx_std_20)

add_executable(chess-server src/main.cpp)
target_link_libraries(chess-server
    PRIVATE
        chess_server_lib
)
