#pragma once


namespace Utility
{
    std::wstring to_wstring( const std::string& s, int code_page );
    std::string to_string( const std::wstring& ws, int code_page );
    std::string to_string( const std::string& s, int code_page_from, int code_page_to );
}

#define WSTRING(s) Utility::to_wstring(s, CP_UTF8)
#define STRING(ws) Utility::to_string(ws, CP_UTF8)
