#ifndef C_SERVER_API_H_INCLUDED
#define C_SERVER_API_H_INCLUDED

#include "c_config.h"
#include "c_shared_data.h"

#ifdef __cplusplus
#ifndef SKIP_LINKAGE
extern "C"
{
#endif // SKIP_LINKAGE
#endif

    #ifndef NO_DS
    struct key_state
    {
        enum state
        {
            UP = 0,
            DOWN = 1,
        };

        state st;
        sized_string key;
    };
    #endif // NO_DS

    DLL_EXPORT sized_string SHIM(sa_make_chat_command, sized_view, sized_view);
    ///does not handle #up
    DLL_EXPORT sized_string SHIM(sa_make_generic_server_command, sized_view);

    DLL_EXPORT sized_string SHIM(sa_make_autocomplete_request, sized_view);

    DLL_EXPORT int SHIM(sa_is_local_command, sized_view);

    ///handles everything for #up, #up_es6, and #dry
    ///you should free server_msg and use the return value
    ///converts #up scriptname -> #up scriptname script_data
    DLL_EXPORT sized_string SHIM(sa_default_up_handling, sized_view, sized_view, sized_view);

    DLL_EXPORT void SHIM(sa_do_poll_server, c_shared_data);
    DLL_EXPORT void SHIM(sa_do_autocomplete_request, c_shared_data, sized_view);
    DLL_EXPORT void SHIM(sa_do_terminate_all_scripts, c_shared_data);
    DLL_EXPORT void SHIM(sa_do_terminate_script, c_shared_data, int);

    ///deprecated, do not use!
    DLL_EXPORT void SHIM(sa_do_send_keystrokes_to_script, c_shared_data, int,
                                         sized_view*, int,
                                         sized_view*, int num_pressed,
                                         sized_view*, int);

    DLL_EXPORT void SHIM(sa_do_send_key_event_stream_to_script, c_shared_data, int,
                                               key_state*, int);

    DLL_EXPORT void SHIM(sa_do_update_mouse_to_script, c_shared_data, int,
                                      float, float,
                                      float,      float);

    DLL_EXPORT void SHIM(sa_do_send_script_info, c_shared_data, int,
                                int, int);

    #ifndef NO_DS
    enum server_command_type
    {
        server_command_command,
        server_command_chat_api,
        server_command_server_scriptargs,
        server_command_server_scriptargs_invalid,
        server_command_server_scriptargs_ratelimit,
        server_command_command_realtime,
        error_invalid_response,
    };

    struct server_command_info
    {
        server_command_type type;
        sized_string data;
    };

    struct realtime_info
    {
        int id;
        sized_string msg;
        int should_close;

        int width;
        int height;
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

    struct tell_info
    {
        sized_string user;
        sized_string msg;
    };

    struct chat_api_info
    {
        chat_info* msgs;
        int num_msgs;

        chat_channel* in_channels;
        int num_in_channels;

        tell_info* tells;
        int num_tells;
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
    #endif // NO_DS

    DLL_EXPORT void SHIM(sa_destroy_server_command_info, server_command_info);
    DLL_EXPORT void SHIM(sa_destroy_realtime_info, realtime_info);
    DLL_EXPORT void SHIM(sa_destroy_chat_api_info, chat_api_info);
    DLL_EXPORT void SHIM(sa_destroy_script_argument_list, script_argument_list);

    DLL_EXPORT server_command_info SHIM(sa_server_response_to_info, sized_view);

    ///server_command_command
    DLL_EXPORT sized_string SHIM(sa_command_to_human_readable, server_command_info);

    DLL_EXPORT realtime_info SHIM(sa_command_realtime_to_info, server_command_info);

    ///server_command_chat_api
    DLL_EXPORT chat_api_info SHIM(sa_chat_api_to_info, server_command_info);

    ///server_command_server_scriptargs
    DLL_EXPORT script_argument_list SHIM(sa_server_scriptargs_to_list, server_command_info);

    ///may return nullptr
    ///server_command_server_scriptargs_invalid
    DLL_EXPORT sized_string SHIM(sa_server_scriptargs_invalid_to_script_name, server_command_info);

    ///server_command_server_scriptargs_ratelimit
    DLL_EXPORT sized_string SHIM(sa_server_scriptargs_ratelimit_to_script_name, server_command_info);

    //DLL_EXPORT void sleep_for_ms(int ms);

#ifdef __cplusplus
#ifndef SKIP_LINKAGE
}
#endif // SKIP_LINKAGE
#endif

#endif // C_SERVER_API_H_INCLUDED
