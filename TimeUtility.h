#pragma once


namespace Utility
{
    std::string string_from_time_t( std::time_t t, const char* format = "%Y/%m/%d %H:%M:%S" );
    std::string duration_string_from_seconds( std::time_t t );
    std::string duration_string_from_time_list( const std::vector<time_t>& times );
    std::vector<time_t> times_from_strings( const std::vector<std::string>& strings );
}
