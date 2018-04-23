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

char* sa_default_up_handling(const char* for_user, const char* server_msg, const char* scripts_dir)
{
    if(for_user == nullptr || server_msg == nullptr || scripts_dir == nullptr)
    {
        printf("sa_default_up_handling error, nullptr argument\n");
        return nullptr;
    }

    std::string up = "#up ";
    std::string up_es6 = "#up_es6 ";
    std::string dry = "#dry ";

    std::string sdir(scripts_dir);
    std::string unknown_command(server_msg);

    std::vector<std::string> strings = no_ss_split(unknown_command, " ");

    if((starts_with(unknown_command, up) || starts_with(unknown_command, dry)) && strings.size() == 3)
    {
        std::string name = strings[2];

        std::string hardcoded_user(for_user);

        std::string diskname = sdir + hardcoded_user + "." + name + ".es5.js";
        std::string diskname_es6 = sdir + hardcoded_user + "." + name + ".js";

        std::string comm = up;

        if(starts_with(unknown_command, dry))
            comm = dry;

        std::string data = "";

        if(file_exists(diskname))
            data = read_file(diskname);

        if(file_exists(diskname_es6))
        {
            data = read_file(diskname_es6);
            comm = up_es6;
        }

        std::string final_command = comm + name + " " + data;

        return cpp_str_to_c(final_command);
    }

    return cpp_str_to_c(unknown_command);
}
