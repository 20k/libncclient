#ifndef C_SHARED_DATA_H_INCLUDED
#define C_SHARED_DATA_H_INCLUDED

#ifdef __cplusplus
extern "C"
{
#endif

    struct shared_data;

    typedef shared_data* c_shared_data;

    ///owning
    #pragma pack(1)
    struct sized_string
    {
        const char* str;
        int num;
    };

    ///non owning
    #pragma pack(1)
    struct sized_view
    {
        const char* str;
        int num;
    };

    __declspec(dllexport) c_shared_data sd_alloc();
    __declspec(dllexport) void sd_destroy(c_shared_data data);

    __declspec(dllexport) void sd_set_auth(c_shared_data data, sized_view auth);
    __declspec(dllexport) sized_string sd_get_auth(c_shared_data data);

    __declspec(dllexport) int sd_has_front_read(c_shared_data data);
    __declspec(dllexport) int sd_has_front_write(c_shared_data data);

    __declspec(dllexport) sized_string sd_get_front_read(c_shared_data data);
    __declspec(dllexport) sized_string sd_get_front_write(c_shared_data data);

    __declspec(dllexport) void sd_add_back_write(c_shared_data data, sized_view write);
    __declspec(dllexport) void sd_add_back_read(c_shared_data data, sized_view read);

    __declspec(dllexport) void sd_set_user(c_shared_data data, sized_view user);
    __declspec(dllexport) sized_string sd_get_user(c_shared_data data);

    __declspec(dllexport) void sd_set_termination(c_shared_data data);
    __declspec(dllexport) int sd_should_terminate(c_shared_data data);

    __declspec(dllexport) void sd_increment_termination_count(c_shared_data data);
    __declspec(dllexport) int sd_get_termination_count(c_shared_data data);

    __declspec(dllexport) void free_string(char*);
    __declspec(dllexport) void free_sized_string(sized_string str);
#ifdef __cplusplus
}
#endif

#endif // C_SHARED_DATA_H_INCLUDED
