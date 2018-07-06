#include "c_net_client.h"
#include "nc_util.hpp"
#include "socket.hpp"
#include "nc_string_interop.hpp"
#include "c_server_api.h"

//#include <crapmud/socket_shared.hpp>

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>

#include <SFML/System.hpp>

namespace
{
//#include <crapmud/script_util_shared.hpp>

using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>
namespace http = boost::beast::http;    // from <boost/beast/http.hpp>

volatile static bool socket_alive = false;

struct shared_context
{
    boost::asio::io_context ioc;

    //tcp::resolver resolver;
    //tcp::socket socket;

    socket_interface* sock = nullptr;

    c_shared_data data;

    ssl::context ctx{ssl::context::sslv23_client};

    bool use_ssl = false;

    shared_context(bool puse_ssl)
    {
        use_ssl = puse_ssl;
    }

    void connect(const std::string& host, const std::string& port)
    {
        if(sock)
            delete sock;

        if(!use_ssl)
        {
            websock_socket_client* tsock = nullptr;
            tsock = new websock_socket_client(ioc);

            auto const results = tsock->resolver.resolve(host, port);

            boost::asio::connect(tsock->ws.next_layer(), results.begin(), results.end());

            #define NAGLE
            #ifdef NAGLE
            boost::asio::ip::tcp::no_delay nagle(true);
            tsock->ws.next_layer().set_option(nagle);
            #endif // NAGLE

            tsock->ws.handshake(host, "/");
            tsock->ws.text(false);

            sock = tsock;
        }
        else
        {
            websock_socket_client_ssl* tsock = nullptr;
            tsock = new websock_socket_client_ssl(ioc, ctx);

            auto const results = tsock->resolver.resolve(host, port);

            boost::asio::connect(tsock->ws.next_layer().next_layer(), results.begin(), results.end());

            #define NAGLE
            #ifdef NAGLE
            boost::asio::ip::tcp::no_delay nagle(true);
            tsock->ws.next_layer().next_layer().set_option(nagle);
            #endif // NAGLE

            tsock->ws.next_layer().handshake(ssl::stream_base::client);

            tsock->ws.handshake(host, "/");
            tsock->ws.text(false);

            sock = tsock;
        }
    }
};

void handle_async_write(c_shared_data shared, shared_context& ctx)
{
    while(1)
    {
        //std::lock_guard<std::mutex> lk(local_mut);
        sf::sleep(sf::milliseconds(8));

        if(sd_should_terminate(shared))
            break;

        try
        {
            if(!socket_alive)
                continue;

            if(sd_has_front_write(shared))
            {
                sized_string c_write = sd_get_front_write(shared);
                std::string next_command = c_str_sized_to_cpp(c_write);
                free_sized_string(c_write);

                if(ctx.sock->write(next_command))
                {
                    socket_alive = false;
                    continue;
                }
            }
        }
        catch(...)
        {
            socket_alive = false;
            std::cout << "caught write exception" << std::endl;
            sf::sleep(sf::milliseconds(1000));
        }
    }

    sd_increment_termination_count(shared);

    printf("write\n");
}

void check_auth(c_shared_data shared, const std::string& str)
{
    std::string auth_str = "command_auth secret ";

    if(str.substr(0, auth_str.length()) == auth_str)
    {
        auto start = str.begin() + auth_str.length();
        std::string key(start, str.end());

        std::string key_file = c_str_consume(sd_get_key_file_name(shared));

        if(!file_exists(key_file))
        {
            write_all_bin(key_file, key);

            sd_set_auth(shared, make_view(key));

            sd_add_back_read(shared, make_view(make_success_col("Success!")));
        }
        else
        {
            printf("Key file already exists");

            sd_add_back_read(shared, make_view(make_error_col("Did not overwrite existing key file, you are already registered")));
        }
    }
}

void handle_async_read(c_shared_data shared, shared_context& ctx)
{
    boost::system::error_code ec;

    while(1)
    {
        sf::sleep(sf::milliseconds(8));

        if(sd_should_terminate(shared))
            break;

        try
        {
            if(!socket_alive)
                continue;

            if(ctx.sock->available() == 0)
                continue;

            if(ctx.sock->read(ec))
            {
                socket_alive = false;
                continue;
            }

            std::string next_command = ctx.sock->get_read();

            check_auth(shared, next_command);
            sd_add_back_read(shared, make_view(next_command));
        }
        catch(...)
        {
            socket_alive = false;
            std::cout << "caught read exception" << std::endl;
            sf::sleep(sf::milliseconds(1000));
        }
    }

    sd_increment_termination_count(shared);

    printf("read\n");
}

void watchdog(c_shared_data shared, shared_context& ctx, const std::string& host_ip, const std::string& host_port)
{
    while(1)
    {
        if(socket_alive)
            sf::sleep(sf::milliseconds(50));

        if(sd_should_terminate(shared))
            break;

        while(!socket_alive)
        {
            if(sd_should_terminate(shared))
                break;

            try
            {
                std::string host = host_ip;
                std::string port = host_port;

                std::cout << "Try Reconnect" << std::endl;

                sd_add_back_read(shared, make_view_from_raw("Connecting..."));

                ctx.connect(host, port);

                sd_add_back_read(shared, make_view_from_raw("`LConnected`"));

                sized_string auth = sd_get_auth(shared);
                std::string auth_str = "client_command auth client " + c_str_sized_to_cpp(auth);
                free_sized_string(auth);

                sd_add_back_write(shared, make_view(auth_str));

                sized_string username = sd_get_user(shared);

                if(username.num > 0)
                {
                    std::string command = "user " + c_str_sized_to_cpp(username);

                    sized_string user_command = sa_make_generic_server_command(make_view(command));

                    sd_add_back_write(shared, make_view(user_command));

                    free_sized_string(user_command);
                }

                free_sized_string(username);

                socket_alive = true;

                sf::sleep(sf::milliseconds(50));
            }
            catch(...)
            {
                sd_add_back_read(shared, make_view_from_raw("`DConnection to the server failed`"));

                std::cout << "Server down" << std::endl;
                sf::sleep(sf::milliseconds(5000));
            }
        }

        sf::sleep(sf::milliseconds(8));
    }

    sd_increment_termination_count(shared);

    printf("watchdog\n");

    while(sd_get_termination_count(ctx.data) != 3){}

    ctx.sock->shutdown();
}
}

__declspec(dllexport) void nc_start(c_shared_data data, const char* host_ip, const char* host_port)
{
    std::string key_file = c_str_consume(sd_get_key_file_name(data));

    if(file_exists(key_file))
    {
        sd_set_auth(data, make_view(read_file_bin(key_file)));
    }

    shared_context* ctx = new shared_context(false);
    ctx->data = data;

    std::string hip(host_ip);
    std::string hpo(host_port);

    std::thread(handle_async_read, data, std::ref(*ctx)).detach();
    std::thread(handle_async_write, data, std::ref(*ctx)).detach();
    std::thread(watchdog, data, std::ref(*ctx), hip, hpo).detach();
}

__declspec(dllexport) void nc_start_ssl(c_shared_data data, const char* host_ip, const char* host_port)
{
    std::string key_file = c_str_consume(sd_get_key_file_name(data));

    if(file_exists(key_file))
    {
        sd_set_auth(data, make_view(read_file_bin(key_file)));
    }

    shared_context* ctx = new shared_context(true);
    ctx->data = data;

    std::string hip(host_ip);
    std::string hpo(host_port);

    std::thread(handle_async_read, data, std::ref(*ctx)).detach();
    std::thread(handle_async_write, data, std::ref(*ctx)).detach();
    std::thread(watchdog, data, std::ref(*ctx), hip, hpo).detach();
}

__declspec(dllexport) void nc_shutdown(c_shared_data data)
{
    sd_set_termination(data);

    while(sd_get_termination_count(data) != 3){}
}
