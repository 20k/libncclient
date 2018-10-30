#ifndef C_SERVER_API_H_INCLUDED
#define C_SERVER_API_H_INCLUDED

#include "c_shared_data.h"

#ifdef __cplusplus
extern "C"
{
#endif
    __declspec(dllexport) sized_string sa_make_chat_command(sized_view chat_channel, sized_view chat_msg);
    ///does not handle #up
    __declspec(dllexport) sized_string sa_make_generic_server_command(sized_view server_msg);

    __declspec(dllexport) sized_string sa_make_autocomplete_request(sized_view scriptname);

    __declspec(dllexport) int sa_is_local_command(sized_view server_msg);

    ///handles everything for #up, #up_es6, and #dry
    ///you should free server_msg and use the return value
    ///converts #up scriptname -> #up scriptname script_data
    __declspec(dllexport) sized_string sa_default_up_handling(sized_view for_user, sized_view server_msg, sized_view scripts_dir);

    __declspec(dllexport) void sa_do_poll_server(c_shared_data data);
    __declspec(dllexport) void sa_do_autocomplete_request(c_shared_data data, sized_view scriptname);
    __declspec(dllexport) void sa_do_terminate_all_scripts(c_shared_data data);
    __declspec(dllexport) void sa_do_terminate_script(c_shared_data data, int script_id);

    ///deprecated, do not use!
    __declspec(dllexport) void sa_do_send_keystrokes_to_script(c_shared_data data, int script_id,
                                         sized_view* keystrokes, int num_keystrokes,
                                         sized_view* on_pressed, int num_pressed,
                                         sized_view* on_released, int num_released);

    __declspec(dllexport) void sa_do_update_mouse_to_script(c_shared_data data, int script_id,
                                      float mousewheel_x, float mousewheel_y,
                                      float mouse_x,      float mouse_y);

    __declspec(dllexport) void sa_do_send_script_info(c_shared_data data, int script_id,
                                int width, int height);

    typedef enum
    {
        server_command_command,
        server_command_chat_api,
        server_command_server_scriptargs,
        server_command_server_scriptargs_invalid,
        server_command_server_scriptargs_ratelimit,
        server_command_command_realtime,
        server_command_command_tagged,
        server_command_command_ping,
        server_command_command_no_pad,
        error_invalid_response,
    } server_command_type;

    typedef struct
    {
        server_command_type type;
        sized_string data;
    } server_command_info;

    typedef struct
    {
        int id;
        sized_string msg;
        int should_close;

        int width;
        int height;
        sized_string name;
    } realtime_info;

    typedef struct
    {
        sized_string channel;
        sized_string msg;
    } chat_info;

    typedef struct
    {
        sized_string channel;
    } chat_channel;

    typedef struct
    {
        sized_string user;
        sized_string msg;
    } tell_info;

    typedef struct
    {
        sized_string msg;
    } notif_info;

    typedef struct
    {
        chat_info* msgs;
        int num_msgs;

        chat_channel* in_channels;
        int num_in_channels;

        tell_info* tells;
        int num_tells;

        notif_info* notifs;
        int num_notifs;

        sized_string current_user;
    } chat_api_info;

    typedef struct
    {
        sized_string key;
        sized_string val;
    } script_argument;

    typedef struct
    {
        sized_string scriptname;
        script_argument* args;
        int num;
    } script_argument_list;

    typedef struct
    {
        sized_string tag;
        sized_string val;
    } command_tagged_info;

    __declspec(dllexport) void sa_destroy_server_command_info(server_command_info info);
    __declspec(dllexport) void sa_destroy_realtime_info(realtime_info info);
    __declspec(dllexport) void sa_destroy_chat_api_info(chat_api_info info);
    __declspec(dllexport) void sa_destroy_script_argument_list(script_argument_list argl);
    __declspec(dllexport) void sa_destroy_command_tagged_info(command_tagged_info info);

    __declspec(dllexport) server_command_info sa_server_response_to_info(sized_view server_response);

    ///server_command_command
    ///also accepts command_no_pad
    __declspec(dllexport) sized_string sa_command_to_human_readable(server_command_info info);

    __declspec(dllexport) realtime_info sa_command_realtime_to_info(server_command_info info);

    ///server_command_chat_api
    __declspec(dllexport) chat_api_info sa_chat_api_to_info(server_command_info info);

    ///server_command_server_scriptargs
    __declspec(dllexport) script_argument_list sa_server_scriptargs_to_list(server_command_info info);

    ///may return nullptr
    ///server_command_server_scriptargs_invalid
    __declspec(dllexport) sized_string sa_server_scriptargs_invalid_to_script_name(server_command_info info);

    ///server_command_server_scriptargs_ratelimit
    __declspec(dllexport) sized_string sa_server_scriptargs_ratelimit_to_script_name(server_command_info info);

    ///server_command_command_tagged
    __declspec(dllexport) command_tagged_info sa_command_tagged_to_info(server_command_info info);

#ifdef __cplusplus
}
#endif

#endif // C_SERVER_API_H_INCLUDED
