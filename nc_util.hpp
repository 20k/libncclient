#ifndef NC_UTIL_HPP_INCLUDED
#define NC_UTIL_HPP_INCLUDED

#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include <algorithm>

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
std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems)
{
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}

inline
std::vector<std::string> split(const std::string &s, char delim)
{
    std::vector<std::string> elems;
    split(s, delim, elems);
    return elems;
}

inline
std::string format_by_vector(std::string to_format, const std::vector<std::string>& all_strings)
{
    int len = 0;

    for(auto& i : all_strings)
    {
        if((int)i.length() > len)
            len = i.length();
    }

    for(int i=(int)to_format.length(); i<len; i++)
    {
        to_format = to_format + " ";
    }

    return to_format;
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

inline
std::string strip_trailing_newlines(std::string in)
{
    while(in.size() > 0 && in.back() == '\n')
        in.pop_back();

    return std::move(in);
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

///ALARM: NEED TO ENFORCE LOWERCASE!!!
inline
bool is_valid_name_character(char c, bool allow_uppercase = false)
{
    if(allow_uppercase)
        return isalnum(c) || c == '_';
    else
        return (isdigit(c) || (isalnum(c) && islower(c))) || c == '_';
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

inline
std::string tolower_str(std::string str)
{
    for(int i=0; i < (int)str.size(); i++)
    {
        str[i] = tolower(str[i]);
    }

    ///no rvo on returning a parameter
    return std::move(str);
}

#define MAX_ANY_NAME_LEN 24

inline
bool is_valid_string(const std::string& to_parse, bool allow_uppercase = false)
{
    if(to_parse.size() >= MAX_ANY_NAME_LEN)
        return false;

    if(to_parse.size() == 0)
        return false;

    bool check_digit = true;

    for(char c : to_parse)
    {
        if(check_digit && isdigit(c))
        {
            return false;
        }

        check_digit = false;

        if(!is_valid_name_character(c, allow_uppercase))
        {
            return false;
        }
    }

    return true;
}

inline
bool is_valid_full_name_string(const std::string& name)
{
    //std::string to_parse = strip_whitespace(name);

    std::string to_parse = name;

    int num_dots = std::count(to_parse.begin(), to_parse.end(), '.');

    if(num_dots != 1)
    {
        return false;
    }

    std::vector<std::string> strings = no_ss_split(to_parse, ".");

    if(strings.size() != 2)
        return false;

    //for(auto& str : strings)
    for(int i=0; i < (int)strings.size(); i++)
    {
        if(!is_valid_string(strings[i], i == 1))
            return false;
    }

    return true;
}

inline
std::string get_script_from_name_string(const std::string& base_dir, const std::string& name_string)
{
    bool is_valid = is_valid_full_name_string(name_string);

    if(!is_valid)
        return "";

    std::string to_parse = strip_whitespace(name_string);

    std::replace(to_parse.begin(), to_parse.end(), '.', '/');

    std::string file = base_dir + "/" + to_parse + ".js";

    if(!file_exists(file))
    {
        return "";
    }

    return read_file(file);
}

inline
std::string make_error_col(const std::string& in)
{
    return "`D" + in + "`";
}

inline
std::string make_success_col(const std::string& in)
{
    return "`L" + in + "`";
}

inline
std::string make_key_col(const std::string& in)
{
    return "`F" + in + "`";
}

inline
std::string make_val_col(const std::string& in)
{
    return "`P" + in + "`";
}

inline
std::string make_notif_col(const std::string& in)
{
    return "`e" + in + "`";
}

inline
std::string make_gray_col(const std::string& in)
{
    return "`b" + in + "`";
}

inline
std::string make_key_val(const std::string& key, const std::string& val)
{
    return make_key_col(key) + ":" + make_val_col(val);
}

inline
std::string string_to_colour(const std::string& in)
{
    if(in == "core")
        return "L";

    if(in == "extern")
        return "H";

    if(tolower_str(in) == "fullsec")
        return "4";

    if(tolower_str(in) == "highsec")
        return "3";

    if(tolower_str(in) == "midsec")
        return "2";

    if(tolower_str(in) == "lowsec")
        return "1";

    if(tolower_str(in) == "nullsec")
        return "0";

    std::string valid_cols = "ABCDEFGHIJKLNOPSTVWXYdefghijlnpqsw";

    size_t hsh = std::hash<std::string>{}(in);

    return std::string(1, valid_cols[(hsh % valid_cols.size())]);
}

inline
std::string colour_string(const std::string& in)
{
    std::string c = string_to_colour(in);

    return "`" + c + in + "`";
}

inline
std::string colour_string_only_alnum(std::string in)
{
    std::string f;

    int first_alpha = -1;

    for(int i=0; i < (int)in.size(); i++)
    {
        if(std::isalnum(in[i]))
        {
            f.push_back(in[i]);

            if(first_alpha == -1)
                first_alpha = i;
        }
    }

    std::string col = string_to_colour(f);

    if(first_alpha >= 0 && first_alpha < (int)in.size())
    {
        in = "`" + col + in;
        //in.insert(first_alpha, "`" + col);
        in += "`";
    }
    else
    {
        return colour_string(in);
    }

    return in;
}

inline
std::string get_host_from_fullname(const std::string& in)
{
    auto found = no_ss_split(in, ".");

    if(found.size() < 1)
        return "";

    return found[0];
}

inline
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

inline
std::string c_str_to_cpp(const char* in)
{
    if(in == nullptr)
        return std::string();

    return std::string(in);
}

inline
std::string c_str_to_cpp(const char* in, int len)
{
    if(in == nullptr)
        return std::string();

    return std::string(in, len);
}

#endif // NC_UTIL_HPP_INCLUDED
