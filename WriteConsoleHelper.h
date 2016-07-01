#pragma once


namespace Utility
{
    struct WriteConsoleHelper {};

    template<typename T> WriteConsoleHelper& operator << ( WriteConsoleHelper& helper, T v )
    {
        std::wstringstream strm;
        strm << v;
        std::wstring ws = strm.str();
        WriteConsole( GetStdHandle(STD_OUTPUT_HANDLE), ws.c_str(), ws.size(), 0, 0 );
        return helper;
    }

    inline WriteConsoleHelper& operator << ( WriteConsoleHelper& helper, const std::wstring& ws )
    {
        WriteConsole( GetStdHandle(STD_OUTPUT_HANDLE), ws.c_str(), ws.size(), 0, 0 );
        return helper;
    }

    inline WriteConsoleHelper& operator << ( WriteConsoleHelper& helper, const wchar_t* ws )
    {
        WriteConsole( GetStdHandle(STD_OUTPUT_HANDLE), ws, wcslen(ws), 0, 0 );
        return helper;
    }

    inline WriteConsoleHelper& operator << ( WriteConsoleHelper& helper, const std::string& s )
    {
        WriteConsoleA( GetStdHandle(STD_OUTPUT_HANDLE), s.c_str(), s.size(), 0, 0 );
        return helper;
    }

    inline WriteConsoleHelper& operator << ( WriteConsoleHelper& helper, const char* s )
    {
        WriteConsoleA( GetStdHandle(STD_OUTPUT_HANDLE), s, strlen(s), 0, 0 );
        return helper;
    }
}

#define stdcout Utility::WriteConsoleHelper()
