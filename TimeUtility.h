#pragma once


namespace Utility
{
    std::wstring string_from_time_t( std::time_t t, const wchar_t* format = L"%Y/%m/%d %H:%M:%S" );
    std::wstring duration_string_from_seconds( std::time_t t );
    std::wstring duration_string_from_time_list( const std::vector<time_t>& times );
    std::vector<time_t> times_from_strings( const std::vector<std::wstring>& strings );
}
