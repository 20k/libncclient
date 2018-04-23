#include "c_shared_data.h"

#include <string>

#include <atomic>
#include <deque>
#include <string>
#include <mutex>

char* cpp_str_to_c(const std::string& str)
{
    int len = str.size() + 1;

    char* ptr = new char[len]();

    for(int i=0; i < (int)str.size(); i++)
    {
        ptr[i] = str[i];
    }

    return ptr;
}

struct shared_data
{
    std::deque<std::string> read_queue;
    std::deque<std::string> write_queue;
    std::string user;

    std::string auth;
    bool send_auth = false;
    volatile bool should_terminate = false;
    std::atomic_int termination_count{0};

    std::mutex ilock;

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

        std::string ret = read_queue.front();

        read_queue.pop_front();

        return ret;
    }

    std::string get_front_write()
    {
        std::lock_guard<std::mutex> lk(ilock);

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
};

c_shared_data sd_alloc()
{
    return new shared_data;
}

void sd_destroy(c_shared_data data)
{
    if(data != nullptr)
        delete data;
}

int sd_has_front_read(c_shared_data data)
{
    return data->has_front_read();
}

int sd_has_front_write(c_shared_data data)
{
    return data->has_front_write();
}

char* sd_get_front_read(c_shared_data data)
{
    return cpp_str_to_c(data->get_front_read());
}

char* sd_get_front_write(c_shared_data data)
{
    return cpp_str_to_c(data->get_front_write());
}

void sd_add_back_write(c_shared_data data, char* write)
{
    if(write == nullptr)
        return;

    std::string str(write);

    return data->add_back_write(str);
}
void sd_add_back_read(c_shared_data data, char* read)
{
    if(read == nullptr)
        return;

    std::string str(read);

    return data->add_back_read(str);
}

void sd_set_user(c_shared_data data, char* user)
{
    std::string str(user);

    return data->set_user(str);
}

char* sd_get_user(c_shared_data data)
{
    return cpp_str_to_c(data->get_user());
}

void free_string(char* c)
{
    delete [] c;
}
