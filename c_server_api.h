#ifndef C_SERVER_API_H_INCLUDED
#define C_SERVER_API_H_INCLUDED

extern "C"
{
    char* sa_make_chat_command(const char* chat_channel, const char* chat_msg);
    ///does not handle #up
    char* sa_make_generic_server_command(const char* server_msg);

    int sa_is_local_command(const char* server_msg);

    ///if this returns a pointer, you should free msg and use the return value
    char* sa_default_up_handling(const char* server_msg, const char* scripts_dir);
}

#endif // C_SERVER_API_H_INCLUDED
