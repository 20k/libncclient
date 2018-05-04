#include "c_server_api.h"
#include "nc_util.hpp"

#include <iostream>
#include <string_view>
#include <cstring>
#include "nc_string_interop.hpp"
#include "deps/json/json.hpp"

sized_string sa_make_chat_command(sized_view chat_channel, sized_view chat_msg)
{
    std::string s1 = c_str_sized_to_cpp(chat_channel);
    std::string s2 = c_str_sized_to_cpp(chat_msg);

    std::string full = "client_chat #hs.msg.send({channel:\"" + s1 + "\", msg:\"" + s2 + "\"})";

    return make_copy(full);
}

sized_string sa_make_generic_server_command(sized_view server_msg)
{
    std::string str = c_str_sized_to_cpp(server_msg);

    std::string full_command = "client_command " + str;

    return make_copy(full_command);
}

sized_string sa_make_autocomplete_request(sized_view server_msg)
{
    std::string str = c_str_sized_to_cpp(server_msg);

    return make_copy("client_scriptargs " + str);
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

int sa_is_local_command(sized_view server_msg)
{
    return is_local_command(c_str_sized_to_cpp(server_msg));
}

sized_string sa_default_up_handling(sized_view for_user, sized_view server_msg, sized_view scripts_dir)
{
    if(for_user.str == nullptr || server_msg.str == nullptr || scripts_dir.str == nullptr)
    {
        printf("sa_default_up_handling error, nullptr argument\n");
        return {};
    }

    std::string up = "#up ";
    std::string up_es6 = "#up_es6 ";
    std::string dry = "#dry ";

    std::string sdir = c_str_sized_to_cpp(scripts_dir);
    std::string unknown_command = c_str_sized_to_cpp(server_msg);

    std::vector<std::string> strings = no_ss_split(unknown_command, " ");

    if((starts_with(unknown_command, up) || starts_with(unknown_command, dry)) && strings.size() == 2)
    {
        std::string name = strings[1];

        std::string hardcoded_user = c_str_sized_to_cpp(for_user);

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

        return make_copy(final_command);
    }

    return make_copy(unknown_command);
}

void sa_do_poll_server(c_shared_data data)
{
    const char* str = "client_poll_json";

    sd_add_back_write(data, make_view_from_raw(str));
}

void sa_do_autocomplete_request(c_shared_data data, sized_view scriptname)
{
    sized_string req = sa_make_autocomplete_request(scriptname);

    sd_add_back_write(data, make_view(req));

    free_sized_string(req);
}

void sa_destroy_server_command_info(server_command_info info)
{
    if(info.data.num == 0)
        return;

    free_sized_string(info.data);
}

server_command_info sa_server_response_to_info(sized_view server_response)
{
    if(server_response.str == nullptr || server_response.num <= 0)
        return {error_invalid_response, {}};

    std::string command_str = "command ";
    std::string chat_api = "chat_api_json ";
    std::string scriptargs = "server_scriptargs ";
    std::string invalid_str = "server_scriptargs_invalid";
    std::string ratelimit_str = "server_scriptargs_ratelimit ";

    std::vector<std::pair<server_command_type, std::string>> dat;

    dat.push_back({server_command_command, command_str});
    dat.push_back({server_command_chat_api, chat_api});
    dat.push_back({server_command_server_scriptargs, scriptargs});
    dat.push_back({server_command_server_scriptargs_invalid, invalid_str});
    dat.push_back({server_command_server_scriptargs_ratelimit, ratelimit_str});

    std::string str = c_str_sized_to_cpp(server_response);

    for(auto& i : dat)
    {
        if(starts_with(str, i.second))
        {
            std::string offset_str = i.second;

            std::string data_str = std::string(str.begin() + offset_str.size(), str.end());

            return {i.first, make_copy(data_str)};
        }
    }

    return {error_invalid_response, {}};
}

sized_string sa_command_to_human_readable(server_command_info info)
{
    if(info.data.num == 0 || info.data.str == nullptr)
        return {};

    std::string cp = c_str_sized_to_cpp(info.data);

    return make_copy(cp);
}

chat_api_info sa_chat_api_to_info(server_command_info info)
{
    if(info.data.num == 0 || info.data.str == nullptr)
        return {};

    std::string chat_in = c_str_sized_to_cpp(info.data);

    using json = nlohmann::json;

    try
    {
        auto full = json::parse(chat_in);

        std::vector<std::string> in_channels = full["channels"].get<std::vector<std::string>>();

        std::vector<std::string> msgs;
        std::vector<std::string> channels;

        json data = full["data"];

        for(int i=0; i < (int)data.size(); i++)
        {
            json element = data[i];

            msgs.push_back(element["text"]);
            channels.push_back(element["channel"]);
        }

        chat_api_info ret = {};
        ret.num_msgs = channels.size();
        ret.num_in_channels = in_channels.size();

        if(channels.size() > 0)
        {
            ret.msgs = new chat_info[ret.num_msgs];

            for(int i=0; i < ret.num_msgs; i++)
            {
                ret.msgs[i].channel = make_copy(channels[i]);
                ret.msgs[i].msg = make_copy(msgs[i]);
            }
        }

        if(in_channels.size() > 0)
        {
            ret.in_channels = new chat_channel[in_channels.size()];

            for(int i=0; i < (int)in_channels.size(); i++)
            {
                ret.in_channels[i].channel = make_copy(in_channels[i]);
            }
        }

        return ret;
    }
    catch(...)
    {
        return {};
    }
}

void sa_destroy_chat_api_info(chat_api_info info)
{
    if(info.num_msgs > 0)
    {
        for(int i=0; i < info.num_msgs; i++)
        {
            free_sized_string(info.msgs[i].channel);
            free_sized_string(info.msgs[i].msg);
        }

        delete [] info.msgs;
    }

    if(info.num_in_channels > 0)
    {
        for(int i=0; i < info.num_in_channels; i++)
        {
            free_sized_string(info.in_channels[i].channel);
        }

        delete [] info.in_channels;
    }
}

void sa_destroy_script_argument_list(script_argument_list argl)
{
    for(int i=0; i < argl.num; i++)
    {
        free_sized_string(argl.args[i].key);
        free_sized_string(argl.args[i].val);
    }

    free_sized_string(argl.scriptname);

    if(argl.args != nullptr)
        delete [] argl.args;
}

script_argument_list sa_server_scriptargs_to_list(server_command_info info)
{
    if(info.data.num == 0 || info.data.str == nullptr)
        return {};

    std::string in = c_str_sized_to_cpp(info.data);

    ///TODO: Make all server/client communication use this format
    std::string_view view(&in[0]);

    //view.remove_prefix(std::min(view.find_first_of(" ")+1, view.size()));

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
        return {};

    std::string scriptname = strings[0];

    strings.erase(strings.begin());

    std::vector<std::pair<std::string, std::string>> args;

    if((strings.size() % 2) != 0)
        return {};

    for(int i=0; i < (int)strings.size(); i+=2)
    {
        std::string key = strings[i];
        std::string arg = strings[i + 1];

        args.push_back({key, arg});
    }

    script_argument_list ret;
    ret.scriptname = make_copy(scriptname);
    ret.args = new script_argument[args.size()];
    ret.num = args.size();

    for(int i=0; i < (int)args.size(); i++)
    {
        ret.args[i].key = make_copy(args[i].first);
        ret.args[i].val = make_copy(args[i].second);
    }

    return ret;
}

sized_string sa_server_scriptargs_invalid_to_script_name(server_command_info info)
{
    if(info.data.str == nullptr || info.data.num == 0)
        return {};

    if(info.data.str[0] != ' ')
        return {};

    std::string full = c_str_sized_to_cpp(info.data);

    std::string str(full.begin() + 1, full.end());

    if(str.size() == 0)
        return {};

    return make_copy(str);
}

sized_string sa_server_scriptargs_ratelimit_to_script_name(server_command_info info)
{
    return make_copy(info.data);
}
