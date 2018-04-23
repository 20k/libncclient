#ifndef NC_STRING_INTEROP_HPP_INCLUDED
#define NC_STRING_INTEROP_HPP_INCLUDED

#include "c_shared_data.h"

inline
std::string c_str_consume(char* c)
{
    if(c == nullptr)
        return std::string();

    std::string ret(c);

    free_string(c);

    return ret;
}

#endif // NC_STRING_INTEROP_HPP_INCLUDED
