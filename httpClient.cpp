//
// Created by jonas on 09.03.21.
//

#include "httpClient.hpp"
#include <boost/beast/http.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>


ConnectionInfo exchangeInfo(const std::string &host, const std::string &port, const std::string &path,
                            ConnectionInfo myInfo) {

    namespace beast = boost::beast;
    namespace http = beast::http;
    namespace net = boost::asio;
    using tcp = net::ip::tcp;

    net::io_context ioc;
    tcp::resolver resolver(ioc);
    beast::tcp_stream stream(ioc);
    auto const results = resolver.resolve(host, port);
    stream.connect(results);
    http::request<http::string_body> req(http::verb::post, path, 11);
    req.set(http::field::host, host);

    nlohmann::json myInfoJson = myInfo;
    req.body() = myInfoJson.dump();
    req.prepare_payload();

    http::write(stream, req);

    beast::flat_buffer buffer;
    http::response<http::string_body> res;
    http::read(stream, buffer, res);

    stream.socket().shutdown(tcp::socket::shutdown_both);

    nlohmann::json theirInfoJson = nlohmann::json::parse(res.body());
    return theirInfoJson.get<ConnectionInfo>();
}
