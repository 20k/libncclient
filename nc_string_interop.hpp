#ifndef NC_STRING_INTEROP_HPP_INCLUDED
#define NC_STRING_INTEROP_HPP_INCLUDED

#include "c_shared_data.h"
#include <cstring>

inline
std::string c_str_consume(char* c)
{
    if(c == nullptr)
        return std::string();

    std::string ret(c);

    free_string(c);

    return ret;
}

inline
sized_string cpp_str_to_sized_c(const std::string& str)
{
    int len = str.size() + 1;

    char* ptr = new char[len]();

    for(int i=0; i < (int)str.size(); i++)
    {
        ptr[i] = str[i];
    }

    return {ptr, len-1};
}

inline
std::string c_str_sized_to_cpp(sized_string str)
{
    std::string ret;
    ret.resize(str.num);

    for(int i=0; i < str.num; i++)
    {
        ret[i] = str.str[i];
    }

    return ret;
}

inline
sized_string sized_str_from_raw(const char* c)
{
    return {c, (int)strlen(c)};
}

#endif // NC_STRING_INTEROP_HPP_INCLUDED
