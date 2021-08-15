#pragma once

#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <map>
#include <algorithm>

using std::vector;
using std::string;
using std::pair;
using std::cout;
using std::endl;
using std::to_string;

/**
 * Check whether a class have member function
 * string ToString();
 */
template<typename T>
struct HasToString
{
    typedef char Yes;
    struct No
    {
        char x[2];
    };

    template<typename U, string (U::*)() = &U::ToString>
    static Yes Check(U*);

    static No Check(...);

    enum
    {
        value = sizeof(Check(static_cast<T*>(nullptr))) == sizeof(Yes)
    };
};

enum class DataType
{
    kBool,
    kInt,
    kFloat,
    kDouble,
    kString,
    kCustom
};

struct ArgData
{
    union Data
    {
        bool bool_data;
        int int_data;
        float float_data;
        double double_data;
    };
    Data data;
    string str_data;
    DataType data_type;
};

/**
 * Function for terminate extract argument package
 */
void Unpack(vector<ArgData>& args_data, int arg_index)
{
}

/**
 * Function for extract argument package
 */
template<typename FirstArg, typename... TailArgs>
void Unpack(vector<ArgData>& args_data, int arg_index, FirstArg&& first_arg, TailArgs&& ...tail_args)
{
    using Type = std::remove_const_t<std::remove_reference_t<FirstArg>>;
    // id constexpr is a kind of static if in compile time
    if constexpr (std::is_same<Type, bool>::value)
    {
        args_data[arg_index].data.bool_data = first_arg;
        args_data[arg_index].data_type = DataType::kBool;
    }
    else if constexpr (std::is_same<Type, int>::value)
    {
        args_data[arg_index].data.int_data = first_arg;
        args_data[arg_index].data_type = DataType::kInt;
    }
    else if constexpr (std::is_same<Type, float>::value)
    {
        args_data[arg_index].data.float_data = first_arg;
        args_data[arg_index].data_type = DataType::kFloat;
    }
    else if constexpr (std::is_same<Type, double>::value)
    {
        args_data[arg_index].data.double_data = first_arg;
        args_data[arg_index].data_type = DataType::kDouble;
    }
    else if constexpr (std::is_same<std::decay_t<Type>, char const*>::value)
    {
        args_data[arg_index].str_data = first_arg;
        args_data[arg_index].data_type = DataType::kString;
    }
    else if constexpr (std::is_same<Type, std::string>::value)
    {
        args_data[arg_index].str_data = first_arg;
        args_data[arg_index].data_type = DataType::kString;
    }
    else if constexpr (std::is_class<Type>::value)
    {
        if constexpr (HasToString<Type>::value)
        {
            args_data[arg_index].str_data = first_arg.ToString();
        }
        else
        {
            args_data[arg_index].str_data = "?";
        }
        args_data[arg_index].data_type = DataType::kCustom;
    }
    arg_index++;
    Unpack(args_data, arg_index, std::forward<TailArgs>(tail_args)...);
}

/**
 * This class is used for decode "{0} {} {1:.2f}"
 */
class Parser
{
public:
    static pair<int, int> GetNextBrackets(const string& s, int begin_index)
    {
        bool begin_found = false;
        int begin = s.size();
        int end = s.size();
        for (int i = begin_index; i < s.size(); i++)
        {
            if (s[i] == '{')
            {
                begin = i;
                begin_found = true;
            }
            if (begin_found and s[i] == '}')
            {
                end = i;
                break;
            }
        }
        return {begin, end};
    }

    struct FormatInfo
    {
        int begin = 0;
        int end = 0;
        bool valid = false;
        bool is_named_index = false;
        int arg_index;
        bool should_format = false;
        int fraction_num = 0;
    };

    // 0-for fail 1-success 2-empty
    static int ParseInteger(const std::string& s, int& result)
    {
        if (s.empty())
        {
            return 2;
        }
        bool valid = std::find_if(s.begin(), s.end(), [](unsigned char c) { return !std::isdigit(c); }) == s.end();
        if (valid)
        {
            result = std::stoi(s);
        }
        return valid;
    }

    static FormatInfo ParseFormatString(const string& str, int default_arg_index)
    {
        FormatInfo info;
        info.valid = true;
        info.arg_index = default_arg_index;
        info.should_format = false;

        int colon_pos = -1;
        int dot_pos = -1;

        for (int i = 0; i < str.size(); i++)
        {
            if (colon_pos < 0 and str[i] == ':')
            {
                colon_pos = i;
            }
            if (dot_pos < 0 and str[i] == '.')
            {
                if (colon_pos < 0)
                {
                    info.valid = false;
                    break;
                }
                dot_pos = i;
                break;
            }
        }

        if ((dot_pos != -1 and colon_pos == -1) or (dot_pos == -1 and colon_pos != -1))
        {
            info.valid = false;
        }

        // if the number before colon is not a number
        if (colon_pos != -1)
        {
            auto parse_result = ParseInteger(str.substr(0, colon_pos), info.arg_index);
            if (parse_result)
            {
                info.is_named_index = parse_result == 1;
            }
            else
            {
                info.valid = false;
            }
        }
        else
        {
            auto parse_result = ParseInteger(str.substr(0, str.size()), info.arg_index);
            if (parse_result)
            {
                info.is_named_index = parse_result == 1;
            }
            else
            {
                info.valid = false;
            }
        }

        // ':' and '.' both exist, we may need to do format
        if (dot_pos != -1 and colon_pos != -1)
        {
            // if there is char between colon_pos and dot_pos, then it is invalid
            if (dot_pos != colon_pos + 1)
            {
                info.valid = false;
            }
            if (str[str.size() - 1] != 'f')
            {
                info.valid = false;
            }
            auto begin = dot_pos + 1;
            if (ParseInteger(str.substr(begin, str.size() - begin - 1), info.fraction_num))
            {
                info.should_format = true;
            }
        }

        if (not info.valid)
        {
            throw "Invalid format of target string";
        }

        return info;
    }
};

/**
 * Format function
 */
template<typename... Args>
string Format(const string& s, Args&& ...args)
{
    std::stringstream sbuf;
    vector<ArgData> args_data(sizeof...(Args));
    int arg_index = 0;
    Unpack(args_data, arg_index, std::forward<Args>(args)...);

    // extract all {***} string structure from s and save it info format_info_vec
    vector<Parser::FormatInfo> format_info_vec;
    int next_begin_index = 0;
    while (true)
    {
        auto result = Parser::GetNextBrackets(s, next_begin_index);
        if (result.second < s.size())
        {
            next_begin_index = result.second + 1;
            auto sub_str = s.substr(result.first + 1, result.second - result.first - 1);
            auto parse_info = Parser::ParseFormatString(sub_str, arg_index);
            parse_info.begin = result.first;
            parse_info.end = result.second;
            if (parse_info.valid)
            {
                format_info_vec.emplace_back(parse_info);
                if (not parse_info.is_named_index)
                {
                    arg_index++;
                }
            }
        }
        else
        {
            break;
        }
    }

    // format the s
    int begin = 0;
    for (auto& format_info:format_info_vec)
    {
        int end = format_info.begin - 1;
        sbuf << s.substr(begin, end - begin + 1);
        if (format_info.arg_index < args_data.size())
        {
            auto& arg = args_data[format_info.arg_index];
            if (arg.data_type == DataType::kBool)
            {
                sbuf << (arg.data.bool_data ? "true" : "false");
            }
            else if (arg.data_type == DataType::kInt)
            {
                sbuf << arg.data.int_data;
            }
            else if (arg.data_type == DataType::kFloat)
            {
                if (format_info.should_format)
                {
                    sbuf.setf(std::ios::fixed);
                    sbuf.precision(format_info.fraction_num);
                }
                sbuf << arg.data.float_data;
            }
            else if (arg.data_type == DataType::kDouble)
            {
                if (format_info.should_format)
                {
                    sbuf.setf(std::ios::fixed);
                    sbuf.precision(format_info.fraction_num);
                }
                sbuf << arg.data.double_data;
            }
            else if (arg.data_type == DataType::kString)
            {
                sbuf << arg.str_data;
            }
            else if (arg.data_type == DataType::kCustom)
            {
                sbuf << arg.str_data;
            }
        }
        begin = format_info.end + 1;
    }
    sbuf << s.substr(begin , s.size() - begin);
    return sbuf.str();
}