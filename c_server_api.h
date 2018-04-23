#ifndef C_SERVER_API_H_INCLUDED
#define C_SERVER_API_H_INCLUDED

#include "c_shared_data.h"

extern "C"
{
    char* sa_make_chat_command(const char* chat_channel, const char* chat_msg);
    ///does not handle #up
    char* sa_make_generic_server_command(const char* server_msg);

    int sa_is_local_command(const char* server_msg);

    ///handles everything for #up, #up_es6, and #dry
    ///you should free server_msg and use the return value
    ///converts #up scriptname -> #up scriptname script_data
    char* sa_default_up_handling(const char* for_user, const char* server_msg, const char* scripts_dir);

    void sa_poll_server(c_shared_data data);
}

#endif // C_SERVER_API_H_INCLUDED
