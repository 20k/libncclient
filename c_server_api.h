#ifndef C_SERVER_API_H_INCLUDED
#define C_SERVER_API_H_INCLUDED

#include "c_shared_data.h"

extern "C"
{
    char* sa_make_chat_command(const char* chat_channel, const char* chat_msg);
    ///does not handle #up
    char* sa_make_generic_server_command(const char* server_msg);

    char* sa_make_autocomplete_request(const char* scriptname);

    int sa_is_local_command(const char* server_msg);

    ///handles everything for #up, #up_es6, and #dry
    ///you should free server_msg and use the return value
    ///converts #up scriptname -> #up scriptname script_data
    char* sa_default_up_handling(const char* for_user, const char* server_msg, const char* scripts_dir);

    void sa_do_poll_server(c_shared_data data);
    void sa_do_autocomplete_request(c_shared_data data, const char* scriptname);

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
        char* data;
        int length;
    };

    void sa_destroy_server_command_info(server_command_info info);

    struct script_argument
    {
        char* key;
        char* val;
    };

    struct script_argument_list
    {
        char* scriptname;
        script_argument* args;
        int num;
    };

    void sa_destroy_script_argument_list(script_argument_list argl);

    server_command_info sa_server_response_to_info(const char* server_response, int response_length);

    ///chat api is the most complex result to parse
    ///so skip that for the moment
    char* sa_command_to_human_readable(server_command_info info);

    script_argument_list sa_server_scriptargs_to_list(server_command_info info);

    ///may return nullptr
    char* sa_server_scriptargs_invalid_to_script_name(server_command_info info);
    char* sa_server_scriptargs_ratelimit_to_script_name(server_command_info info);
}

#endif // C_SERVER_API_H_INCLUDED
