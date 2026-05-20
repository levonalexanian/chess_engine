#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <chrono>
#include <cstdint>
#include <cstring>
#include <future>
#include <stdexcept>
#include <string>
#include <thread>

#include <gtest/gtest.h>

#include "chess_server/app.h"

static std::uint16_t pick_free_port() {
    int sock = ::socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        throw std::runtime_error("socket() failed");
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = 0;

    if (::bind(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
        ::close(sock);
        throw std::runtime_error("bind() failed");
    }

    socklen_t len = sizeof(addr);
    if (::getsockname(sock, reinterpret_cast<sockaddr*>(&addr), &len) != 0) {
        ::close(sock);
        throw std::runtime_error("getsockname() failed");
    }

    std::uint16_t port = ntohs(addr.sin_port);
    ::close(sock);
    return port;
}

static std::string http_get(std::string const& host, std::uint16_t port, std::string const& path) {
    int sock = ::socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        throw std::runtime_error("socket() failed");
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (::inet_pton(AF_INET, host.c_str(), &addr.sin_addr) != 1) {
        ::close(sock);
        throw std::runtime_error("inet_pton() failed");
    }

    if (::connect(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
        ::close(sock);
        throw std::runtime_error("connect() failed");
    }

    std::string request = "GET " + path + " HTTP/1.1\r\nHost: " + host + "\r\nConnection: close\r\n\r\n";
    if (::send(sock, request.data(), request.size(), 0) < 0) {
        ::close(sock);
        throw std::runtime_error("send() failed");
    }

    std::string response;
    char buffer[4096];
    while (true) {
        ssize_t n = ::recv(sock, buffer, sizeof(buffer), 0);
        if (n < 0) {
            ::close(sock);
            throw std::runtime_error("recv() failed");
        }
        if (n == 0) {
            break;
        }
        response.append(buffer, static_cast<std::size_t>(n));
    }
    ::close(sock);
    return response;
}

namespace detail {

struct RunningApp {
    chess_server::App app;
    std::future<void> run_future;

    explicit RunningApp(chess_server::AppOptions options) : app(std::move(options)) {
        run_future = app.crow_app().bindaddr("127.0.0.1").port(app.options().port).run_async();
        app.crow_app().wait_for_server_start();
    }

    ~RunningApp() {
        app.crow_app().stop();
        if (run_future.valid()) {
            run_future.wait();
        }
    }
};

}  // namespace detail

TEST(ServerSkeleton, HealthzReturnsOkJson) {
    chess_server::AppOptions options;
    options.bind_address = "127.0.0.1";
    options.port = pick_free_port();
    options.static_root = "/nonexistent/frontend/dist";

    detail::RunningApp runner(options);

    std::string response = http_get("127.0.0.1", options.port, "/healthz");

    EXPECT_NE(response.find("HTTP/1.1 200"), std::string::npos)
        << "expected 200 status line, got: " << response;

    auto body_start = response.find("\r\n\r\n");
    ASSERT_NE(body_start, std::string::npos) << "no body separator in: " << response;
    std::string body = response.substr(body_start + 4);

    EXPECT_NE(body.find("\"status\""), std::string::npos) << "body missing status key: " << body;
    EXPECT_NE(body.find("\"ok\""), std::string::npos) << "body missing ok value: " << body;
}

TEST(ServerSkeleton, RootServesStubWhenFrontendMissing) {
    chess_server::AppOptions options;
    options.bind_address = "127.0.0.1";
    options.port = pick_free_port();
    options.static_root = "/nonexistent/frontend/dist";

    detail::RunningApp runner(options);

    std::string response = http_get("127.0.0.1", options.port, "/");
    EXPECT_NE(response.find("HTTP/1.1 200"), std::string::npos)
        << "expected 200 status line, got: " << response;
    EXPECT_NE(response.find("chess-server"), std::string::npos);
}

TEST(ServerSkeleton, RejectsPathTraversal) {
    chess_server::AppOptions options;
    options.bind_address = "127.0.0.1";
    options.port = pick_free_port();
    options.static_root = "/tmp";

    detail::RunningApp runner(options);

    std::string response = http_get("127.0.0.1", options.port, "/..%2Fetc%2Fpasswd");
    EXPECT_TRUE(response.find("HTTP/1.1 400") != std::string::npos ||
                response.find("HTTP/1.1 404") != std::string::npos)
        << "expected 400/404 for traversal attempt, got: " << response;
}
