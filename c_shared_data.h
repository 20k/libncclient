#ifndef C_SHARED_DATA_H_INCLUDED
#define C_SHARED_DATA_H_INCLUDED

#include "c_config.h"

#ifdef __cplusplus
extern "C"
{
#endif

    struct shared_data;

    typedef shared_data* c_shared_data;

    ///owning
    struct sized_string
    {
        const char* str;
        int num;
    };

    ///non owning
    struct sized_view
    {
        const char* str;
        int num;
    };

    DLL_EXPORT c_shared_data sd_alloc();
    DLL_EXPORT void sd_destroy(c_shared_data data);

    DLL_EXPORT void sd_set_auth(c_shared_data data, sized_view auth);
    DLL_EXPORT sized_string sd_get_auth(c_shared_data data);

    DLL_EXPORT int sd_has_front_read(c_shared_data data);
    DLL_EXPORT int sd_has_front_write(c_shared_data data);

    DLL_EXPORT sized_string sd_get_front_read(c_shared_data data);
    DLL_EXPORT sized_string sd_get_front_write(c_shared_data data);

    DLL_EXPORT void sd_add_back_write(c_shared_data data, sized_view write);
    DLL_EXPORT void sd_add_back_read(c_shared_data data, sized_view read);

    DLL_EXPORT void sd_set_user(c_shared_data data, sized_view user);
    DLL_EXPORT sized_string sd_get_user(c_shared_data data);

    DLL_EXPORT void sd_set_termination(c_shared_data data);
    DLL_EXPORT int sd_should_terminate(c_shared_data data);

    DLL_EXPORT void sd_increment_termination_count(c_shared_data data);
    DLL_EXPORT int sd_get_termination_count(c_shared_data data);

    DLL_EXPORT void free_string(char*);
    DLL_EXPORT void free_sized_string(sized_string str);
#ifdef __cplusplus
}
#endif

#endif // C_SHARED_DATA_H_INCLUDED
