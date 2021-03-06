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

    return make_copy("client_scriptargs_json " + str);
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
        std::string diskname_ts = sdir + hardcoded_user + "." + name + ".ts";

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

        if(file_exists(diskname_ts))
        {
            data = read_file(diskname_ts);
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

void sa_do_terminate_all_scripts(c_shared_data data)
{
    std::string command = "client_terminate_scripts";

    using nlohmann::json;

    json j;
    j["id"] = -1;

    sd_add_back_write(data, make_view(command + " " + j.dump()));
}

void sa_do_terminate_script(c_shared_data data, int script_id)
{
    std::string command = "client_terminate_scripts";

    using nlohmann::json;

    json j;
    j["id"] = script_id;

    sd_add_back_write(data, make_view(command + " " + j.dump()));
}

void sa_do_send_keystrokes_to_script(c_shared_data data, int script_id,
                                     sized_view* keystrokes, int num_keystrokes,
                                     sized_view* on_pressed, int num_pressed,
                                     sized_view* on_released, int num_released)
{
    std::string command = "client_script_keystrokes ";

    using nlohmann::json;

    std::vector<std::string> keys;
    std::vector<std::string> pressed;
    std::vector<std::string> released;

    for(int i=0; i < num_keystrokes; i++)
    {
        keys.push_back(c_str_sized_to_cpp(keystrokes[i]));
    }

    for(int i=0; i < num_pressed; i++)
    {
        pressed.push_back(c_str_sized_to_cpp(on_pressed[i]));
    }

    for(int i=0; i < num_released; i++)
    {
        released.push_back(c_str_sized_to_cpp(on_released[i]));
    }

    json j;

    j["id"] = script_id;
    j["input_keys"] = keys;
    j["released_keys"] = released;
    j["pressed_keys"] = pressed;

    std::string full_command = command + j.dump();

    sd_add_back_write(data, make_view(full_command));
}

void sa_do_update_mouse_to_script(c_shared_data data, int script_id,
                                  float mousewheel_x, float mousewheel_y,
                                  float mouse_x,      float mouse_y)
{
    std::string command = "client_script_mouseinput ";

    using nlohmann::json;

    json j;

    j["id"] = script_id;
    j["mouse_x"] = mouse_x;
    j["mouse_y"] = mouse_y;
    j["mousewheel_x"] = mousewheel_x;
    j["mousewheel_y"] = mousewheel_y;

    std::string full_command = command + j.dump();

    sd_add_back_write(data, make_view(full_command));
}

void sa_do_send_script_info(c_shared_data data, int script_id, int width, int height)
{
    std::string command = "client_script_info ";

    using nlohmann::json;

    json j;
    j["id"] = script_id;

    j["width"] = width;
    j["height"] = height;

    std::string full_command = command + j.dump();

    sd_add_back_write(data, make_view(full_command));
}

void sa_destroy_server_command_info(server_command_info info)
{
    if(info.data.num == 0)
        return;

    free_sized_string(info.data);
}

void sa_destroy_realtime_info(realtime_info info)
{
    free_sized_string(info.msg);
    free_sized_string(info.name);
}

void sa_destroy_command_tagged_info(command_tagged_info info)
{
    free_sized_string(info.tag);
    free_sized_string(info.val);
}

void sa_destroy_command_down_info(command_down_info info)
{
    free_sized_string(info.full_name);
    free_sized_string(info.script_data);
}

server_command_info sa_server_response_to_info(sized_view server_response)
{
    if(server_response.str == nullptr || server_response.num <= 0)
        return {error_invalid_response, {}};

    std::string command_str = "command ";
    std::string command_no_pad_str = "command_no_pad ";
    std::string realtime_str = "command_realtime_json ";
    std::string chat_api = "chat_api_json ";
    std::string scriptargs = "server_scriptargs_json ";
    std::string invalid_str = "server_scriptargs_invalid_json";
    std::string ratelimit_str = "server_scriptargs_ratelimit_json ";
    std::string command_tagged_str = "command_tagged ";
    std::string command_ping_str = "command_ping";
    std::string command_down_str = "command_down ";
    std::string chat_api_response_str = "chat_api_response ";

    std::vector<std::pair<server_command_type, std::string>> dat;

    dat.push_back({server_command_command, command_str});
    dat.push_back({server_command_command_realtime, realtime_str});
    dat.push_back({server_command_chat_api, chat_api});
    dat.push_back({server_command_server_scriptargs, scriptargs});
    dat.push_back({server_command_server_scriptargs_invalid, invalid_str});
    dat.push_back({server_command_server_scriptargs_ratelimit, ratelimit_str});
    dat.push_back({server_command_command_tagged, command_tagged_str});
    dat.push_back({server_command_command_ping, command_ping_str});
    dat.push_back({server_command_command_no_pad, command_no_pad_str});
    dat.push_back({server_command_command_down, command_down_str});
    dat.push_back({server_command_chat_api_response, chat_api_response_str});

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

realtime_info sa_command_realtime_to_info(server_command_info info)
{
    realtime_info ret;

    std::string str = c_str_sized_to_cpp(info.data);

    using json = nlohmann::json;

    try
    {
        json j = json::parse(str);

        int id = j["id"];

        ret.id = id;

        if(j.find("msg") != j.end())
        {
            std::string msg = j["msg"];
            ret.msg = make_copy(msg);
        }
        else
        {
            ret.msg = make_copy("");
        }

        if(j.find("close") != j.end())
        {
            ret.should_close = j["close"];
        }
        else
        {
            ret.should_close = 0;
        }

        if(j.find("width") != j.end())
        {
            ret.width = j["width"];
        }
        else
        {
            ret.width = 0;
        }

        if(j.find("height") != j.end())
        {
            ret.height = j["height"];
        }
        else
        {
            ret.height = 0;
        }

        if(j.find("script_name") != j.end())
        {
            std::string name = j["script_name"];

            ret.name = make_copy(name);
        }
        else
        {
            ret.name = make_copy("");
        }

        return ret;
    }
    catch(...)
    {
        return {};
    }
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

        std::vector<std::string> notif_msgs = full["notifs"];

        std::vector<std::string> chat_msgs;
        std::vector<std::string> chat_channels;

        json chat_data = full["data"];

        for(int i=0; i < (int)chat_data.size(); i++)
        {
            json element = chat_data[i];

            chat_channels.push_back(element["channel"]);
            chat_msgs.push_back(element["text"]);
        }

        std::vector<std::string> tell_users;
        std::vector<std::string> tell_msgs;

        json tell_data = full["tells"];

        for(int i=0; i < (int)tell_data.size(); i++)
        {
            json element = tell_data[i];

            tell_users.push_back(element["user"]);
            tell_msgs.push_back(element["text"]);
        }

        chat_api_info ret = {};
        ret.num_msgs = chat_channels.size();
        ret.num_in_channels = in_channels.size();
        ret.num_tells = tell_users.size();
        ret.num_notifs = notif_msgs.size();

        if(chat_channels.size() > 0)
        {
            ret.msgs = new chat_info[ret.num_msgs];

            for(int i=0; i < ret.num_msgs; i++)
            {
                ret.msgs[i].channel = make_copy(chat_channels[i]);
                ret.msgs[i].msg = make_copy(chat_msgs[i]);
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

        if(tell_users.size() > 0)
        {
            ret.tells = new tell_info[tell_users.size()];

            for(int i=0; i < (int)tell_users.size(); i++)
            {
                ret.tells[i].msg = make_copy(tell_msgs[i]);
                ret.tells[i].user = make_copy(tell_users[i]);
            }
        }

        if(notif_msgs.size() > 0)
        {
            ret.notifs = new notif_info[notif_msgs.size()];

            for(int i=0; i < (int)notif_msgs.size(); i++)
            {
                ret.notifs[i].msg = make_copy(notif_msgs[i]);
            }
        }

        if(full.count("user") > 0)
        {
            std::string cpp_user = full["user"];

            ret.current_user = make_copy(cpp_user);
        }
        else
        {
            ret.current_user = make_copy("");
        }

        if(full.count("root_user") > 0)
        {
            std::string cpp_user = full["root_user"];

            ret.root_user = make_copy(cpp_user);
        }
        else
        {
            ret.root_user = make_copy("");
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

    if(info.num_tells > 0)
    {
        for(int i=0; i < info.num_tells; i++)
        {
            free_sized_string(info.tells[i].user);
            free_sized_string(info.tells[i].msg);
        }

        delete [] info.tells;
    }

    if(info.num_notifs > 0)
    {
        for(int i=0; i < info.num_notifs; i++)
        {
            free_sized_string(info.notifs[i].msg);
        }

        delete [] info.notifs;
    }

    free_sized_string(info.current_user);
    free_sized_string(info.root_user);
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

    try
    {
        using json = nlohmann::json;

        json data = json::parse(in);

        ///script
        ///keys
        ///vals

        std::string name = data["script"].get<std::string>();
        std::vector<std::string> keys = data["keys"].get<std::vector<std::string>>();
        std::vector<std::string> vals = data["vals"].get<std::vector<std::string>>();

        if(keys.size() != vals.size())
        {
            throw std::runtime_error("keys.size() != vals.size()");
        }

        script_argument_list ret;
        ret.scriptname = make_copy(name);
        ret.args = new script_argument[keys.size()];
        ret.num = keys.size();

        for(int i=0; i < (int)keys.size(); i++)
        {
            ret.args[i].key = make_copy(keys[i]);
            ret.args[i].val = make_copy(vals[i]);
        }

        return ret;
    }
    catch(...)
    {
        return {};
    }
}

sized_string sa_server_scriptargs_invalid_to_script_name(server_command_info info)
{
    if(info.data.str == nullptr || info.data.num == 0)
        return {};

    if(info.data.str[0] != ' ')
        return {};

    std::string str = c_str_sized_to_cpp(info.data);

    if(str.size() == 0)
        return {};

    ///remove front ' '
    str.erase(str.begin());

    try
    {
        using json = nlohmann::json;

        json data = json::parse(str);

        return make_copy(data["script"].get<std::string>());
    }
    catch(...)
    {
        return {};
    }
}

sized_string sa_server_scriptargs_ratelimit_to_script_name(server_command_info info)
{
    std::string str = c_str_sized_to_cpp(info.data);

    try
    {
        using json = nlohmann::json;

        json data = json::parse(str);

        return make_copy(data["script"].get<std::string>());
    }
    catch(...)
    {
        return {};
    };
}

command_tagged_info sa_command_tagged_to_info(server_command_info info)
{
    if(info.data.str == nullptr || info.data.num == 0)
        return {};

    std::string str = c_str_sized_to_cpp(info.data);

    std::string tag;

    for(int i=0; i < str.size() && str[i] != ' '; i++)
    {
        tag += str[i];
    }

    int idx = tag.size() + 1;

    std::string val;

    for(int i=idx; i < str.size(); i++)
    {
        val += str[i];
    }

    command_tagged_info ret;
    ret.tag = make_copy(tag);
    ret.val = make_copy(val);

    return ret;
}

command_down_info sa_command_down_to_info(server_command_info info)
{
    if(info.data.str == nullptr || info.data.num == 0)
        return {};

    std::string str = c_str_sized_to_cpp(info.data);

    std::string full_name;

    for(int i=0; i < str.size() && str[i] != ' '; i++)
    {
        full_name += str[i];
    }

    int idx = full_name.size() + 1;

    std::string script_data;

    for(int i=idx; i < str.size(); i++)
    {
        script_data += str[i];
    }

    command_down_info ret;
    ret.full_name = make_copy(full_name);
    ret.script_data = make_copy(script_data);

    return ret;
}

