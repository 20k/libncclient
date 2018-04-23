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

    return cpp_str_to_c(str);
}
