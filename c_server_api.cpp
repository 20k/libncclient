#include "c_server_api.h"
#include "nc_util.hpp"

#include <iostream>
#include <string_view>

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

char* sa_make_autocomplete_request(const char* server_msg)
{
    std::string str = c_str_to_cpp(server_msg);

    return cpp_str_to_c("client_scriptargs " + str);
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

    if((starts_with(unknown_command, up) || starts_with(unknown_command, dry)) && strings.size() == 2)
    {
        std::string name = strings[1];

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

void sa_do_poll_server(c_shared_data data)
{
    const char* str = "client_poll";

    sd_add_back_write(data, str);
}

void sa_do_autocomplete_request(c_shared_data data, const char* scriptname)
{
    char* req = sa_make_autocomplete_request(scriptname);

    sd_add_back_write(data, req);

    free_string(req);
}

void sa_destroy_server_command_info(server_command_info info)
{
    if(info.data == nullptr)
        return;

    free_string(info.data);
}

server_command_info sa_server_response_to_info(const char* server_response, int response_length)
{
    if(server_response == nullptr)
        return {error_invalid_response, nullptr, 0};

    std::string command_str = "command ";
    std::string chat_api = "chat_api ";
    std::string scriptargs = "server_scriptargs ";
    std::string invalid_str = "server_scriptargs_invalid";
    std::string ratelimit_str = "server_scriptargs_ratelimit ";

    std::vector<std::pair<server_command_type, std::string>> dat;

    dat.push_back({server_command_command, command_str});
    dat.push_back({server_command_chat_api, chat_api});
    dat.push_back({server_command_server_scriptargs, scriptargs});
    dat.push_back({server_command_server_scriptargs_invalid, invalid_str});
    dat.push_back({server_command_server_scriptargs_ratelimit, ratelimit_str});

    std::string str(server_response, response_length);

    for(auto& i : dat)
    {
        if(starts_with(str, i.second))
        {
            std::string offset_str = i.second;

            std::string data_str = std::string(str.begin() + offset_str.size(), str.end());

            return {i.first, cpp_str_to_c(data_str), response_length - (int)data_str.size()};
        }
    }

    return {error_invalid_response, nullptr, 0};
}

char* sa_command_to_human_readable(server_command_info info)
{
    if(info.data == nullptr)
        return nullptr;

    std::string cp(info.data, info.length);

    return cpp_str_to_c(cp.c_str());
}

void sa_destroy_script_argument_list(script_argument_list argl)
{
    for(int i=0; i < argl.num; i++)
    {
        free_string(argl.args[i].key);
        free_string(argl.args[i].val);
    }

    free_string(argl.args);
}

script_argument_list sa_server_scriptargs_to_list(server_command_info info)
{
    std::string in(info.data, info.length);

    ///TODO: Make all server/client communication use this format
    std::string_view view(&in[0]);

    view.remove_prefix(std::min(view.find_first_of(" ")+1, view.size()));

    std::vector<std::string> strings;

    while(view.size() > 0)
    {
        auto found = view.find(" ");

        if(found == std::string_view::npos)
            break;

        std::string_view num(view.data(), found);

        std::string len(num);

        int ilen = stoi(len);

        std::string dat(view.substr(len.size() + 1, ilen));

        strings.push_back(dat);

        ///"len" + " " + len_bytes + " "
        view.remove_prefix(len.size() + 2 + ilen);
    }

    if(strings.size() == 0)
        return {nullptr, 0};

    std::string scriptname = strings[0];

    strings.erase(strings.begin());

    std::vector<std::pair<std::string, std::string>> args;

    if((strings.size() % 2) != 0)
        return {nullptr, 0};

    for(int i=0; i < (int)strings.size(); i+=2)
    {
        std::string key = strings[i];
        std::string arg = strings[i + 1];

        args.push_back({key, arg});
    }

    script_argument_list ret;
    ret.args = new script_argument[args.size()];
    ret.num = args.size();

    for(int i=0; i < (int)args.size(); i++)
    {
        ret.args[i].key = cpp_str_to_c(args[i].first);
        ret.args[i].val = cpp_str_to_c(args[i].second);
    }

    return ret;
}
