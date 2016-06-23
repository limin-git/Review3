#include "stdafx.h"
#include "UnicodeUtility.h"


namespace Utility
{

    std::wstring to_wstring( const std::string& s, int code_page )
    {
        static wchar_t buffer[ 1024 * 1024];
        MultiByteToWideChar( code_page, 0, s.c_str(), - 1, buffer, 1024 * 1024 );
        return std::wstring( buffer );
    }


    std::string to_string( const std::wstring& ws, int code_page )
    {
        static char buffer[ 1024 * 1024];
        WideCharToMultiByte( code_page, 0, ws.c_str(), -1, buffer, 1024 * 1024, 0, 0 );
        return std::string( buffer );
    }


    std::string to_string( const std::string& s, int code_page_from, int code_page_to )
    {
        static char buffer[ 1024 * 1024];
        static wchar_t wbuffer[ 1024 * 1024];
        int wsize = MultiByteToWideChar( code_page_from, 0, s.c_str(), -1, wbuffer, 1024 * 1024 );
        WideCharToMultiByte( code_page_to, 0, wbuffer, wsize, buffer, 1024 * 1024, 0, 0 );
        return std::string( buffer );
    }

}
