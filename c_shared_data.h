#ifndef C_SHARED_DATA_H_INCLUDED
#define C_SHARED_DATA_H_INCLUDED

extern "C"
{
    struct shared_data;

    typedef shared_data* c_shared_data;

    c_shared_data sd_alloc();
    void sd_destroy(c_shared_data data);

    void sd_set_auth(c_shared_data data, const char* auth);

    int sd_has_front_read(c_shared_data data);
    int sd_has_front_write(c_shared_data data);

    char* sd_get_front_read(c_shared_data data);
    char* sd_get_front_write(c_shared_data data);

    void sd_add_back_write(c_shared_data data, const char* write);
    void sd_add_back_read(c_shared_data data, const char* read);

    void sd_set_user(c_shared_data data, const char* user);
    char* sd_get_user(c_shared_data data);

    void free_string(char*);
}

#endif // C_SHARED_DATA_H_INCLUDED
