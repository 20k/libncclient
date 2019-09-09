#ifndef SOCKET_SHARED_HPP_INCLUDED
#define SOCKET_SHARED_HPP_INCLUDED

#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include "nc_util.hpp"
#include <mutex>
#include <SFML/System.hpp>
#include <iostream>

#ifdef SERVER
#include "../../logging.hpp"
#endif // SERVER

using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>
namespace http = boost::beast::http;    // from <boost/beast/http.hpp>
namespace websocket = boost::beast::websocket;
namespace ssl = boost::asio::ssl;               // from <boost/asio/ssl.hpp>

// Report a failure
inline
void
fail(boost::system::error_code ec, char const* what)
{
    std::cerr << what << ": " << ec.message() << "\n";
}

// This is the C++11 equivalent of a generic lambda.
// The function object is used to send an HTTP message.
template<class Stream>
struct send_lambda
{
    Stream& stream_;
    bool& close_;
    boost::system::error_code& ec_;

    explicit
    send_lambda(
        Stream& stream,
        bool& close,
        boost::system::error_code& ec)
        : stream_(stream)
        , close_(close)
        , ec_(ec)
    {
    }

    template<bool isRequest, class Body, class Fields>
    void
    operator()(http::message<isRequest, Body, Fields>&& msg) const
    {
        // Determine if we should close the connection after
        close_ = msg.need_eof();

        // We need the serializer here because the serializer requires
        // a non-const file_body, and the message oriented version of
        // http::write only works with const messages.
        http::serializer<isRequest, Body, Fields> sr{msg};
        http::write(stream_, sr, ec_);
    }
};

struct socket_interface
{
    sf::Clock timeout_clock;
    std::recursive_mutex mut;

    virtual bool write(const std::string& msg) {return false;};
    virtual bool read(boost::system::error_code& ec){return false;};

    virtual std::string get_read(){return std::string();};

    virtual void shutdown(){};

    virtual bool is_open(){return false;};

    virtual int available(){return 0;}

    virtual void ping(const std::string& payload){}

    void restart_timeout()
    {
        std::lock_guard guard(mut);

        timeout_clock.restart();
    }

    bool timed_out()
    {
        std::lock_guard guard(mut);

        return timeout_clock.getElapsedTime().asSeconds() > 30;
    }

    virtual ~socket_interface(){}
};

struct http_socket : socket_interface
{
    tcp::socket socket;

    boost::beast::flat_buffer buffer;
    http::request<http::string_body> req;

    send_lambda<tcp::socket> lambda;//{socket, close, ec};

    boost::system::error_code lec;
    bool close = false;

    http_socket(tcp::socket&& sock) : socket(std::move(sock)), lambda{socket, close, lec} {}

    virtual bool read(boost::system::error_code& ec) override
    {
        std::lock_guard guard(mut);

        req = http::request<http::string_body>();
        buffer = boost::beast::flat_buffer();

        http::read(socket, buffer, req, ec);

        if(ec == http::error::end_of_stream)
            return true;
        if(ec)
        {
            fail(ec, "read");
            return true;
        }

        restart_timeout();

        return false;
    }

    virtual std::string get_read() override
    {
        std::lock_guard guard(mut);

        return req.body();
    }

    virtual bool write(const std::string& msg) override
    {
        std::lock_guard guard(mut);

        http::response<http::string_body> res{
                    std::piecewise_construct,
                    std::make_tuple(std::move(msg)),
                    std::make_tuple(http::status::ok, 11)};

        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.keep_alive(true);

        res.set(http::field::content_type, "text/plain");
        res.prepare_payload();

        lambda(std::move(res));

        return false;
    }

    virtual void shutdown() override
    {
        socket.shutdown(tcp::socket::shutdown_send, lec);
    }

    virtual bool is_open() override
    {
        return socket.is_open();
    }

    virtual int available() override
    {
        return socket.available();
    }
};

#define WS_COMPRESSION
#define NAGLE

struct websock_socket : socket_interface
{
    boost::beast::websocket::stream<tcp::socket> ws;
    boost::beast::multi_buffer mbuffer;
    boost::system::error_code lec;

    websock_socket(tcp::socket&& sock) : ws(std::move(sock))
    {
        #ifdef NAGLE
        boost::asio::ip::tcp::no_delay nagle(true);
        ws.next_layer().set_option(nagle);
        #endif // NAGLE

        boost::beast::websocket::permessage_deflate opt;
        //opt.client_enable = true; // for clients

        #ifdef WS_COMPRESSION
        opt.server_enable = true; // for servers
        ws.set_option(opt);
        #endif // COMPRESSION

        ws.accept();
    }

    websock_socket(boost::asio::io_context& ioc) : ws{ioc} {}

    virtual bool read(boost::system::error_code& ec) override
    {
        std::lock_guard guard(mut);

        mbuffer = decltype(mbuffer)();

        ws.read(mbuffer, ec);
        ws.text(ws.got_text());

        if(ec)
        {
            fail(ec, "read");
            return true;
        }

        restart_timeout();

        return false;
    }

    std::string get_read() override
    {
        std::lock_guard guard(mut);

        return boost::beast::buffers_to_string(mbuffer.data());
    }

    virtual bool write(const std::string& msg) override
    {
        std::lock_guard guard(mut);

        //ws.text(true);

        ws.write(boost::asio::buffer(msg), lec);

        if(lec)
        {
            fail(lec, "write");
            return true;
        }

        return false;
    }

    virtual void shutdown() override
    {
        std::lock_guard guard(mut);

        ws.close(boost::beast::websocket::close_code::normal, lec);
    }

    virtual bool is_open() override
    {
        std::lock_guard guard(mut);

        return ws.is_open();
    }

    virtual int available() override
    {
        std::lock_guard guard(mut);

        return ws.next_layer().available();
    }

    virtual void ping(const std::string& payload) override
    {
        std::lock_guard guard(mut);

        ws.ping(payload.c_str());
    }

    virtual ~websock_socket(){}
};

struct ssl_ctx_wrap
{
    ssl::context ctx{ssl::context::sslv23};

    ssl_ctx_wrap(bool init)
    {
        if(!init)
            return;

        #ifdef SERVER
        lg::log("Pre ssl ctx wrap");
        #endif // SERVER

        ///remember to deploy these!
        ///certpw1234
        static std::string cert = read_file_bin("./deps/secret/cert/cert.crt");
        static std::string dh = read_file_bin("./deps/secret/cert/dh.pem");
        static std::string key = read_file_bin("./deps/secret/cert/key.pem");

        ctx.set_options(boost::asio::ssl::context::default_workarounds |
                        boost::asio::ssl::context::no_sslv2 |
                        boost::asio::ssl::context::single_dh_use |
                        boost::asio::ssl::context::no_sslv3);

        ctx.use_certificate_chain(
            boost::asio::buffer(cert.data(), cert.size()));

        ctx.use_private_key(
            boost::asio::buffer(key.data(), key.size()),
            boost::asio::ssl::context::file_format::pem);

        ctx.use_tmp_dh(
            boost::asio::buffer(dh.data(), dh.size()));

        #ifdef SERVER
        lg::log("Post ssl ctx wrap");
        #endif // SERVER
    }
};

struct websock_socket_ssl : socket_interface
{
    ssl_ctx_wrap ctx;
    boost::beast::websocket::stream<ssl::stream<tcp::socket>> ws;
    boost::beast::multi_buffer mbuffer;
    boost::system::error_code lec;

    websock_socket_ssl(tcp::socket&& sock) : ctx(true), ws{std::move(sock), ctx.ctx}
    {
        #ifdef SERVER
        lg::log("start ssl constructor websock");
        #endif // SERVER

        #ifdef NAGLE
        boost::asio::ip::tcp::no_delay nagle(true);
        ws.next_layer().next_layer().set_option(nagle);
        #endif // NAGLE

        boost::beast::websocket::permessage_deflate opt;
        //opt.client_enable = true; // for clients

        #ifdef WS_COMPRESSION
        opt.server_enable = true; // for servers
        ws.set_option(opt);
        #endif // COMPRESSION

        ws.next_layer().handshake(ssl::stream_base::server);

        ws.accept();

        #ifdef SERVER
        lg::log("fin ssl constructor websock");
        #endif // SERVER
    }

    websock_socket_ssl(boost::asio::io_context& ioc, ssl::context& lctx) : ctx(false), ws{ioc, lctx} {}

    virtual bool read(boost::system::error_code& ec) override
    {
        std::lock_guard guard(mut);

        mbuffer = decltype(mbuffer)();

        ws.read(mbuffer, ec);
        ws.text(ws.got_text());

        restart_timeout();

        if(ec)
        {
            fail(ec, "read");
            return true;
        }

        return false;
    }

    std::string get_read() override
    {
        std::lock_guard guard(mut);

        return boost::beast::buffers_to_string(mbuffer.data());
    }

    virtual bool write(const std::string& msg) override
    {
        std::lock_guard guard(mut);

        //ws.text(true);

        ws.write(boost::asio::buffer(msg), lec);

        if(lec)
        {
            fail(lec, "write");
            return true;
        }

        return false;
    }

    virtual void shutdown() override
    {
        std::lock_guard guard(mut);

        ws.close(boost::beast::websocket::close_code::normal, lec);
    }

    virtual bool is_open() override
    {
        std::lock_guard guard(mut);

        return ws.is_open();
    }

    virtual int available() override
    {
        std::lock_guard guard(mut);

        return ws.next_layer().next_layer().available();
    }

    virtual void ping(const std::string& payload) override
    {
        std::lock_guard guard(mut);

        ws.ping(payload.c_str());
    }

    virtual ~websock_socket_ssl(){}
};

struct websock_socket_client : websock_socket
{
    tcp::resolver resolver;

    websock_socket_client(boost::asio::io_context& ioc) : websock_socket(ioc), resolver{ioc}
    {
        boost::beast::websocket::permessage_deflate opt;

        #ifdef WS_COMPRESSION
        opt.client_enable = true; // for clients
        //opt.server_enable = true; // for servers
        ws.set_option(opt);
        #endif // WS_COMPRESSION

        ws.text(false);
    }

    virtual ~websock_socket_client(){}
};

struct websock_socket_client_ssl : websock_socket_ssl
{
    tcp::resolver resolver;

    websock_socket_client_ssl(boost::asio::io_context& ioc, ssl::context& ctx) : websock_socket_ssl(ioc, ctx), resolver{ioc}
    {
        boost::beast::websocket::permessage_deflate opt;

        #ifdef WS_COMPRESSION
        opt.client_enable = true; // for clients
        //opt.server_enable = true; // for servers
        ws.set_option(opt);
        #endif // WS_COMPRESSION

        ws.text(false);
    }

    virtual ~websock_socket_client_ssl(){}
};

#endif // SOCKET_SHARED_HPP_INCLUDED
