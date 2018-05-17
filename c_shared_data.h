#ifndef C_SHARED_DATA_H_INCLUDED
#define C_SHARED_DATA_H_INCLUDED

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

    c_shared_data sd_alloc();
    void sd_destroy(c_shared_data data);

    void sd_set_auth(c_shared_data data, sized_view auth);
    sized_string sd_get_auth(c_shared_data data);

    int sd_has_front_read(c_shared_data data);
    int sd_has_front_write(c_shared_data data);

    sized_string sd_get_front_read(c_shared_data data);
    sized_string sd_get_front_write(c_shared_data data);

    void sd_add_back_write(c_shared_data data, sized_view write);
    void sd_add_back_read(c_shared_data data, sized_view read);

    void sd_set_user(c_shared_data data, sized_view user);
    sized_string sd_get_user(c_shared_data data);

    void sd_set_termination(c_shared_data data);
    int sd_should_terminate(c_shared_data data);

    void sd_increment_termination_count(c_shared_data data);
    int sd_get_termination_count(c_shared_data data);

    void free_string(char*);
    void free_sized_string(sized_string str);
#ifdef __cplusplus
}
#endif

#endif // C_SHARED_DATA_H_INCLUDED
