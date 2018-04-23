#include "c_server_api.h"
#include "nc_util.hpp"

char* sa_make_chat_command(const char* chat_channel, const char* chat_msg)
{
    std::string s1 = c_str_to_cpp(chat_channel);
    std::string s2 = c_str_to_cpp(chat_msg);

    std::string full = "client_chat #hs.msg.send({channel:\"" + s1 + "\", msg:\"" + s2 + "\"})";

    return cpp_str_to_c(full);
}

char* sa_make_generic_server_command(const char* server_msg)
{
    std::string str = c_str_to_cpp(server_msg);

    std::string full_command = "client_command " + str;

    return cpp_str_to_c(full_command);
}

bool is_local_command(const std::string& command)
{
    if(command == "#")
        return true;

    if(starts_with(command, "#edit "))
        return true;

    if(starts_with(command, "#edit_es6 "))
        return true;

    if(starts_with(command, "#edit_es5 "))
        return true;

    if(starts_with(command, "#open "))
        return true;

    if(starts_with(command, "#dir"))
        return true;

    if(starts_with(command, "#clear_autos"))
        return true;

    if(starts_with(command, "#autos_clear"))
        return true;

    if(starts_with(command, "#shutdown"))
        return true;

    if(starts_with(command, "#cls"))
        return true;

    if(starts_with(command, "#clear_term"))
        return true;

    if(starts_with(command, "#clear_chat"))
        return true;

    return false;
}

int sa_is_local_command(const char* server_msg)
{
    return is_local_command(server_msg);
}
