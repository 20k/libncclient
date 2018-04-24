#ifndef C_SERVER_API_H_INCLUDED
#define C_SERVER_API_H_INCLUDED

#include "c_shared_data.h"

extern "C"
{
    sized_string sa_make_chat_command(sized_view chat_channel, sized_view chat_msg);
    ///does not handle #up
    sized_string sa_make_generic_server_command(sized_view server_msg);

    sized_string sa_make_autocomplete_request(sized_view scriptname);

    int sa_is_local_command(sized_view server_msg);

    ///handles everything for #up, #up_es6, and #dry
    ///you should free server_msg and use the return value
    ///converts #up scriptname -> #up scriptname script_data
    sized_string sa_default_up_handling(sized_view for_user, sized_view server_msg, sized_view scripts_dir);

    void sa_do_poll_server(c_shared_data data);
    void sa_do_autocomplete_request(c_shared_data data, sized_view scriptname);

    enum server_command_type
    {
        server_command_command,
        server_command_chat_api,
        server_command_server_scriptargs,
        server_command_server_scriptargs_invalid,
        server_command_server_scriptargs_ratelimit,
        error_invalid_response,
    };

    struct server_command_info
    {
        server_command_type type;
        sized_string data;
    };

    struct chat_info
    {
        sized_string channel;
        sized_string msg;
    };

    struct chat_channel
    {
        sized_string channel;
    };

    struct chat_api_info
    {
        chat_info* msgs;
        int num_msgs;

        chat_channel* in_channels;
        int num_in_channels;
    };

    struct script_argument
    {
        sized_string key;
        sized_string val;
    };

    struct script_argument_list
    {
        sized_string scriptname;
        script_argument* args;
        int num;
    };

    void sa_destroy_server_command_info(server_command_info info);
    void sa_destroy_chat_api_info(chat_api_info info);
    void sa_destroy_script_argument_list(script_argument_list argl);

    server_command_info sa_server_response_to_info(sized_string server_response);

    ///server_command_command
    sized_string sa_command_to_human_readable(server_command_info info);

    ///server_command_chat_api
    chat_api_info sa_chat_api_to_info(server_command_info info);

    ///server_command_server_scriptargs
    script_argument_list sa_server_scriptargs_to_list(server_command_info info);

    ///may return nullptr
    ///server_command_server_scriptargs_invalid
    sized_string sa_server_scriptargs_invalid_to_script_name(server_command_info info);

    ///server_command_server_scriptargs_ratelimit
    sized_string sa_server_scriptargs_ratelimit_to_script_name(server_command_info info);
}

#endif // C_SERVER_API_H_INCLUDED
