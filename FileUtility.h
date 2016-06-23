#pragma once


namespace Utility
{
    std::wstring wstring_from_file( const wchar_t* file_name, int code_page = CP_ACP );
    std::string string_from_file( const wchar_t* file_name, int file_code_page = CP_ACP, int string_code_page = CP_UTF8 );
}
