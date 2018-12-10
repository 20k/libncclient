#include "c_shared_data.h"
#include "nc_util.hpp"
#include "nc_string_interop.hpp"

#include <string>

#include <atomic>
#include <deque>
#include <string>
#include <mutex>

struct shared_data
{
    std::deque<std::string> read_queue;
    std::deque<std::string> write_queue;
    std::string user;

    std::string auth;
    volatile bool should_terminate = false;
    std::atomic_int termination_count{0};
    std::string key_file = "key.key";
    bool use_steam_auth = true;

    std::mutex ilock;

    void set_steam_auth(bool puse_steam_auth)
    {
        std::lock_guard<std::mutex> lk(ilock);

        use_steam_auth = puse_steam_auth;
    }

    bool should_use_steam_auth()
    {
        std::lock_guard<std::mutex> lk(ilock);

        return use_steam_auth;
    }

    bool has_key_auth()
    {
        std::lock_guard<std::mutex> lk(ilock);

        return auth.size() != 0;
    }

    void make_lock()
    {
        ilock.lock();
    }

    void make_unlock()
    {
        ilock.unlock();
    }

    bool has_front_read()
    {
        std::lock_guard<std::mutex> lk(ilock);

        return read_queue.size() > 0;
    }

    bool has_front_write()
    {
        std::lock_guard<std::mutex> lk(ilock);

        return write_queue.size() > 0;
    }

    std::string get_front_read()
    {
        std::lock_guard<std::mutex> lk(ilock);

        ///it might seem a little odd to produce an error string here
        ///instead of doing something more sensible, but given that this function
        ///will primarily display to the terminal, it will make it
        ///extremely obvious if something has gone wrong
        if(read_queue.size() == 0)
            return "Catastrophic error in get_front_read(), queue is empty";

        std::string ret = read_queue.front();

        read_queue.pop_front();

        return ret;
    }

    std::string get_front_write()
    {
        std::lock_guard<std::mutex> lk(ilock);

        if(write_queue.size() == 0)
            return "Catstrophic error in get_front_write(), queue is empty";

        std::string ret = write_queue.front();

        write_queue.pop_front();

        return ret;
    }

    void add_back_write(const std::string& str)
    {
        std::lock_guard<std::mutex> lk(ilock);

        write_queue.push_back(str);
    }

    void add_back_read(const std::string& str)
    {
        std::lock_guard<std::mutex> lk(ilock);

        read_queue.push_back(str);
    }

    void set_user(const std::string& in)
    {
        std::lock_guard<std::mutex> lk(ilock);

        user = in;
    }

    std::string get_user()
    {
        std::lock_guard<std::mutex> lk(ilock);

        return user;
    }

    void set_key_file(const std::string& fname)
    {
        std::lock_guard<std::mutex> lk(ilock);

        key_file = fname;
    }

    std::string get_key_file()
    {
        std::lock_guard<std::mutex> lk(ilock);

        return key_file;
    }
};

__declspec(dllexport) c_shared_data sd_alloc()
{
    return new shared_data;
}

__declspec(dllexport) void sd_destroy(c_shared_data data)
{
    if(data != nullptr)
        delete data;
}

__declspec(dllexport) void sd_set_auth(c_shared_data data, sized_view auth)
{
    if(auth.str == nullptr)
        return;

    data->auth = c_str_sized_to_cpp(auth);
}

__declspec(dllexport) sized_string sd_get_auth(c_shared_data data)
{
    return make_copy(data->auth);
}

__declspec(dllexport) void sd_set_key_file_name(c_shared_data data, sized_view key)
{
    shared_data* cdata = (shared_data*)data;

    cdata->set_key_file(c_str_sized_to_cpp(key));
}

__declspec(dllexport) sized_string sd_get_key_file_name(c_shared_data data)
{
    shared_data* cdata = (shared_data*)data;

    return make_copy(cdata->get_key_file());
}

__declspec(dllexport) int sd_has_key_auth(c_shared_data data)
{
    shared_data* cdata = (shared_data*)data;

    return cdata->has_key_auth();
}

__declspec(dllexport) int sd_has_front_read(c_shared_data data)
{
    return data->has_front_read();
}

__declspec(dllexport) int sd_has_front_write(c_shared_data data)
{
    return data->has_front_write();
}

__declspec(dllexport) sized_string sd_get_front_read(c_shared_data data)
{
    return make_copy(data->get_front_read());
}

__declspec(dllexport) sized_string sd_get_front_write(c_shared_data data)
{
    return make_copy(data->get_front_write());
}

__declspec(dllexport) void sd_add_back_write(c_shared_data data, sized_view write)
{
    if(write.str == nullptr)
        return;

    std::string str = c_str_sized_to_cpp(write);

    return data->add_back_write(str);
}

__declspec(dllexport) void sd_add_back_read(c_shared_data data, sized_view read)
{
    if(read.str == nullptr)
        return;

    std::string str = c_str_sized_to_cpp(read);

    return data->add_back_read(str);
}

__declspec(dllexport) void sd_set_user(c_shared_data data, sized_view user)
{
    std::string str = c_str_sized_to_cpp(user);

    return data->set_user(str);
}

__declspec(dllexport) sized_string sd_get_user(c_shared_data data)
{
    return make_copy(data->get_user());
}

__declspec(dllexport) void sd_set_termination(c_shared_data data)
{
    data->should_terminate = true;

    while(data->termination_count != 3){}
}

__declspec(dllexport) int sd_should_terminate(c_shared_data data)
{
    return data->should_terminate;
}

__declspec(dllexport) void sd_increment_termination_count(c_shared_data data)
{
    data->termination_count++;
}

__declspec(dllexport) int sd_get_termination_count(c_shared_data data)
{
    return data->termination_count;
}

__declspec(dllexport) void sd_set_use_steam_auth(c_shared_data data, int use_steam_auth)
{
    data->set_steam_auth(use_steam_auth);
}

__declspec(dllexport) int sd_use_steam_auth(c_shared_data data)
{
    return data->should_use_steam_auth();
}

__declspec(dllexport) void free_string(char* c)
{
    if(c == nullptr)
        return;

    delete [] c;
}

__declspec(dllexport) void free_sized_string(sized_string str)
{
    if(str.str == nullptr)
        return;

    delete [] str.str;
}
