#pragma once


class Console
{
public:

    static HANDLE stdoutput()
    {
        static HANDLE output = GetStdHandle( STD_OUTPUT_HANDLE );
        return output;
    }

    static HANDLE stdinput()
    {
        static HANDLE input = GetStdHandle( STD_INPUT_HANDLE );
        return input;
    }

    static void show_console_cursor( BOOL visible = true, HANDLE output_handle = stdoutput() )
    {
        CONSOLE_CURSOR_INFO cursor_info;
        GetConsoleCursorInfo( output_handle, &cursor_info );

        if ( visible != cursor_info.bVisible )
        {
            cursor_info.bVisible = visible;
            SetConsoleCursorInfo( output_handle, &cursor_info );
        }
    }

    static std::wstring to_wstring( const std::string& s, int code_page )
    {
        static wchar_t buffer[ 1024 * 1024];
        MultiByteToWideChar( code_page , 0, s. c_str(), - 1, buffer , 1024 * 1024 );
        return std::wstring( buffer );
    }

    static std::string to_string( const std::wstring& ws, int code_page )
    {
        static char buffer[ 1024 * 1024];
        WideCharToMultiByte( code_page , 0, ws. c_str(), - 1, buffer , 1024 * 1024, 0, 0 );
        return std::string( buffer );
    }

    static void refresh_console_window( HANDLE output_handle = stdoutput() )
    {
        CONSOLE_SCREEN_BUFFER_INFO csbinfo;
        GetConsoleScreenBufferInfo( output_handle, &csbinfo );
        COORD coord = { 0, 0 };
        CHAR_INFO char_fill;
        char_fill.Char.UnicodeChar = L' ';
        ScrollConsoleScreenBuffer( output_handle, &csbinfo.srWindow, NULL, coord, &char_fill  );
    }

    static void print_font_info( HANDLE output_handle = stdoutput() )
    {
        CONSOLE_FONT_INFOEX f;
        f.cbSize = sizeof( CONSOLE_FONT_INFOEX );

        if ( 0 == GetCurrentConsoleFontEx( output_handle, TRUE, &f ) )
        {
            std::cout << "failed: " << GetLastError() << std::endl;
            return;
        }

        std::wcout
            << L"nFont     : " << f.nFont << std::endl
            << L"dwFontSize: " << f.dwFontSize.X << L"," << f.dwFontSize.Y << std::endl
            << L"FontFamily: " << f.FontFamily << std::endl
            << L"FontWeight: " << f.FontWeight << std::endl
            << L"FaceName  : " << f.FaceName << std::endl;
    }

    static void set_console_font( SHORT font_size = 14, const std::wstring& face_name = L"Consolas", HANDLE output_handle = stdoutput() )
    {
        // nFont: (no use)
        // 14 Console
        // 15 Lucida Console
        // 6  新宋体
        // dwFontSize   Consolas    Lucida Console  新宋体
        // 10:          5,  10      6,  10          5,  11
        // 12:          6,  12      7,  12          6,  14
        // 14:          7,  14      8,  14          7,  16
        // 16:          8,  16      10, 16          8,  18
        // 18:          8,  18      11, 18          9,  21
        // 20:          9,  20      12, 20          10, 23
        // 24:          11, 24      14, 24          12, 27
        // 28:          13, 28      17, 28          14, 32
        // 36:          17, 36      22, 36          18, 41
        // FaceName:
        // Lucida Console
        // Consolas
        // Terminal
        // 新宋体

        SetConsoleOutputCP( face_name == L"新宋体" ? 936 : CP_UTF8 ); // DO NOT use CP_ACP
        CONSOLE_FONT_INFOEX f;
        f.cbSize = sizeof( CONSOLE_FONT_INFOEX );
        f.nFont = 0;
        f.dwFontSize.X = 0;
        f.dwFontSize.Y = font_size;
        f.FontFamily = FF_DONTCARE;
        f.FontWeight = FW_NORMAL;
        wcscpy_s( f.FaceName, face_name.c_str() );

        if ( 0 == SetCurrentConsoleFontEx( output_handle, FALSE, &f ) )
        {
            std::cout << "failed: " << GetLastError() << std::endl;
            return;
        }
    }

    static void disable_console_system_buttons()
    {   
        HWND w = GetConsoleWindow();
        HMENU m = GetSystemMenu( w, FALSE );
        DeleteMenu( m, SC_CLOSE , MF_BYCOMMAND );
        DeleteMenu( m, SC_MINIMIZE , MF_BYCOMMAND );
        DeleteMenu( m, SC_MAXIMIZE , MF_BYCOMMAND );
        DrawMenuBar( w );
    }

    static void set_console_color( WORD color, HANDLE output_handle = stdoutput() )
    {
        DWORD written = 0;
        COORD coord = { 0, 0 };
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        GetConsoleScreenBufferInfo( output_handle, &csbi );
        csbi.wAttributes = color;
        SetConsoleTextAttribute( output_handle, csbi.wAttributes );
        FillConsoleOutputAttribute( output_handle, csbi.wAttributes, csbi.dwSize.X * csbi.dwSize.Y, coord, &written ); 
    }

    static void set_console_window_size( SHORT col, SHORT row, HANDLE output_handle = stdoutput() )
    {
        COORD coord = { 0, 0 };
        COORD size = { col, row };
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        GetConsoleScreenBufferInfo( output_handle, &csbi );

        // make sure buffer size is bigger window size
        COORD cur_size = { csbi.srWindow.Right - csbi.srWindow.Left + 1, csbi.srWindow.Bottom - csbi.srWindow.Top + 1 };
        COORD tmp_buf_size = { std::max<int>(cur_size.X, col), std::max<int>(cur_size.Y, row) };
        SetConsoleScreenBufferSize( output_handle, tmp_buf_size );

        SMALL_RECT new_window = csbi.srWindow;
        new_window.Right = new_window.Left + col - 1;
        new_window.Bottom = new_window.Top + row - 1;
        SetConsoleWindowInfo( output_handle, TRUE, &new_window );

        SetConsoleScreenBufferSize( output_handle, size );
    }

    static int WINAPI console_ctrl_handler( DWORD  dwCtrlType )
    {
        std::cout << static_cast<char>(7) << std::endl;

        switch ( dwCtrlType )
        {
        case CTRL_C_EVENT:
            std::cout << "Ctrl + C" << std::endl;
            break;
        case CTRL_BREAK_EVENT:
            std::cout << "Ctrl + BREAK" << std::endl;
            break;
        case CTRL_CLOSE_EVENT:
            std::cout << "CLOSE EVENT" << std::endl;
            break;
        case CTRL_LOGOFF_EVENT:
            std::cout << "LOGOFF EVENT" << std::endl;
            break;
        case CTRL_SHUTDOWN_EVENT:
            std::cout << "SHUTDOWN EVENT" << std::endl;
            break;
        }

        return TRUE;
    }

    static void set_console_ctrl_handler()
    {
        SetConsoleCtrlHandler( console_ctrl_handler, TRUE );
    }


    static void disable_console_mode( DWORD disable_mode, HANDLE handle = GetStdHandle(STD_INPUT_HANDLE) )
    {
        // must call this before SetConsoleMode
        DWORD mode = 0;
        GetConsoleMode( handle, &mode );
        mode &= ~disable_mode;
        SetConsoleMode( handle, mode );
    }

    static void cls( HANDLE output_handle = stdoutput() )
    {
        COORD coord = { 0, 0 };
        CONSOLE_SCREEN_BUFFER_INFOEX csbi;
        csbi.cbSize = sizeof( CONSOLE_SCREEN_BUFFER_INFOEX );
        GetConsoleScreenBufferInfoEx( output_handle, &csbi );
        size_t buf_size = csbi.dwSize.X * csbi.dwSize.Y;
        CHAR_INFO* buf = new CHAR_INFO[buf_size];
        for ( size_t i = 0; i < buf_size; ++i )
        {
            buf[i].Attributes = csbi.wAttributes;
            buf[i].Char.UnicodeChar = L' ';
        }
        SMALL_RECT wrige_region = { 0, 0, csbi.dwSize.X - 1, csbi.dwSize.Y - 1 };
        WriteConsoleOutput( output_handle, buf, csbi.dwSize, coord, &wrige_region );
        delete[] buf;
        buf = NULL;
    }

    static void cls2( HANDLE output_handle = stdoutput() )
    {
        DWORD written = 0;
        COORD coord = { 0, 0 };
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        GetConsoleScreenBufferInfo( output_handle, &csbi );
        size_t buf_size = csbi.dwSize.X * csbi.dwSize.Y;
        wchar_t* buf = new wchar_t[buf_size];
        for ( size_t i = 0; i < buf_size; ++i )
        {
            buf[i] = L' ';
        }
        WriteConsoleOutputCharacterW( output_handle, buf, buf_size, coord, &written );
    }

    static void write_console_on_center( const std::string& s, HANDLE output = GetStdHandle( STD_OUTPUT_HANDLE ) )
    {
        std::wstring ws = to_wstring( s, CP_UTF8 );
        std::string as = to_string( ws, 936 );

        DWORD written = 0;
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        GetConsoleScreenBufferInfo( output, &csbi );
        size_t window_width = csbi.srWindow.Right - csbi.srWindow.Left + 1;
        size_t window_height = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;

        if ( 0 == ( window_height % 2 ) )
        {
            --window_height;
        }

        size_t pos = ( window_width * window_height - as.size() ) / 2;
        COORD coord = { pos % window_width, pos / window_width };
        WriteConsoleOutputCharacterW( output, ws.c_str(), ws.size(), coord, &written );
    }

};


inline std::ostream& operator <<( std::ostream& os, const COORD& c )
{
    return os << "(" << c.X << ", " << c.Y << ")";
}

inline std::ostream& operator <<( std::ostream& os, const SMALL_RECT& s )
{
    return os << "(" << s.Left << ", " << s.Top << ", " << s.Right << ", " << s.Bottom << ")";
}

inline std::ostream& operator <<( std::ostream& os, const CONSOLE_SCREEN_BUFFER_INFO& c )
{
    return os
        << "dwSize: " << c.dwSize << std::endl
        << "dwCursorPosition: " << c.dwCursorPosition << std::endl 
        << "wAttributes: " << c.wAttributes << std::endl 
        << "srWindow: " << c.srWindow << std::endl 
        << "dwMaximumWindowSize: " << c.dwMaximumWindowSize;
}
