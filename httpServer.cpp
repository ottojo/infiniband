//
// Created by jonas on 09.03.21.
//

#include "httpServer.hpp"

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/config.hpp>
#include <iostream>
#include <fmt/core.h>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;

void fail(beast::error_code ec, char const *what) {
    std::cerr << what << ": " << ec.message() << "\n";
    throw std::exception{};
}

ConnectionInfo serveInfo(unsigned short port, ConnectionInfo myInfo) {

    const auto address = net::ip::make_address("0.0.0.0");

    net::io_context ioc{1};

    // The acceptor receives incoming connections
    tcp::acceptor acceptor{ioc, {address, port}};
    // This will receive the new connection
    tcp::socket socket{ioc};

    // Block until we get a connection
    acceptor.accept(socket);

    // Launch the session, transferring ownership of the socket
    beast::error_code ec;

    // This buffer is required to persist across reads
    beast::flat_buffer buffer;

    // Read a request
    http::request<http::string_body> req;
    http::read(socket, buffer, req, ec);
    if (ec) {
        fail(ec, "read");
    }


    // Send the response
    http::response<http::string_body> res{http::status::ok, req.version()};
    res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(http::field::content_type, "application/json");

    nlohmann::json j = myInfo;
    res.body() = j.dump();
    res.prepare_payload();

    http::serializer<false, http::string_body> sr{res};
    http::write(socket, sr, ec);
    if (ec) {
        fail(ec, "write");
    }

    // Send a TCP shutdown
    socket.shutdown(tcp::socket::shutdown_send, ec);
    nlohmann::json theirInfoJson = nlohmann::json::parse(req.body());
    return theirInfoJson.get<ConnectionInfo>();
}
