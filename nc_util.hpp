#ifndef NC_UTIL_HPP_INCLUDED
#define NC_UTIL_HPP_INCLUDED

#include <string>
#include <sstream>
#include <fstream>
#include <vector>

inline
std::string read_file(const std::string& file)
{
    std::ifstream t(file);
    std::string str((std::istreambuf_iterator<char>(t)),
                     std::istreambuf_iterator<char>());

    return str;
}

inline
void write_all(const std::string& fname, const std::string& str)
{
    std::ofstream out(fname);
    out << str;
}

inline
void write_all_bin(const std::string& fname, const std::string& str)
{
    std::ofstream out(fname, std::ios::binary);
    out << str;
}

inline
std::string read_file_bin(const std::string& file)
{
    std::ifstream t(file, std::ios::binary);
    std::string str((std::istreambuf_iterator<char>(t)),
                     std::istreambuf_iterator<char>());

    return str;
}

inline
bool file_exists(const std::string& name)
{
    std::ifstream f(name.c_str());
    return f.good();
}

inline
std::string strip_whitespace(std::string in)
{
    if(in.size() == 0)
        return in;

    while(in.size() > 0 && isspace(in[0]))
    {
        in.erase(in.begin());
    }

    while(in.size() > 0 && isspace(in.back()))
    {
        in.pop_back();
    }

    return in;
}

template<typename T>
inline
bool starts_with(const T& in, const std::string& test)
{
    if(in.size() < test.size())
        return false;

    if(in.substr(0, test.length()) == test)
        return true;

    return false;
}

///i think something is broken with 7.2s stringstream implementation
///i dont know why the stringstream version crashes
inline
std::vector<std::string> no_ss_split(const std::string& str, const std::string& delim)
{
    std::vector<std::string> tokens;
    size_t prev = 0, pos = 0;
    do
    {
        pos = str.find(delim, prev);
        if (pos == std::string::npos) pos = str.length();
        std::string token = str.substr(prev, pos-prev);
        if (!token.empty()) tokens.push_back(token);
        prev = pos + delim.length();
    }
    while (pos < str.length() && prev < str.length());
    return tokens;
}


#endif // NC_UTIL_HPP_INCLUDED
