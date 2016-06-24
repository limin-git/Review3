#include "stdafx.h"
#include "ConsoleCommand.h"
#include "OptionString.h"
#include "OptionUpdateHelper.h"
#include "ProgramOptions.h"
#include "Utility.h"


ConsoleCommand::ConsoleCommand()
{
    m_handle = GetStdHandle( STD_OUTPUT_HANDLE );
    m_cp = GetConsoleCP();
    m_output_cp = GetConsoleOutputCP();

    show_console_cursor( false );
    disable_console_system_buttons();

    ProgramOptions::connect_to_signal( boost::bind( &ConsoleCommand::update_option, this, _1 ) );
}


ConsoleCommand::~ConsoleCommand()
{
    SetConsoleCP( m_cp );
    SetConsoleOutputCP( m_output_cp );
}


void ConsoleCommand::update_option( const boost::program_options::variables_map& vm )
{
    static OptionUpdateHelper option_helper;

    if ( option_helper.update_one_option<std::wstring>( system_font_face_name, vm ) )
    {
        std::wstring font_face_name = option_helper.get_value<std::wstring>( system_font_face_name );
        set_font_face_name( font_face_name );
        LOG_DEBUG << "font-face-name: " << font_face_name;
    }

    if ( option_helper.update_one_option<SHORT>( system_font_size, vm ) )
    {
        SHORT font_size = option_helper.get_value<SHORT>( system_font_size );
        set_font_size( font_size );
        LOG_DEBUG << "font-size: " << font_size;
    }

    if ( option_helper.update_one_option<SHORT>( system_console_width, vm ) )
    {
        SHORT width = option_helper.get_value<SHORT>( system_console_width );
        set_console_width( width );
        LOG_DEBUG << "console-width: " << width;
    }

    if ( option_helper.update_one_option<SHORT>( system_console_height, vm ) )
    {
        SHORT height = option_helper.get_value<SHORT>( system_console_height );
        set_console_height( height );
        LOG_DEBUG << "console-height: " << height;
    }

    if ( option_helper.update_one_option<std::wstring>( system_console_color, vm ) )
    {
        std::wstring color_str = option_helper.get_value<std::wstring>( system_console_color );
        WORD color = 0;
        std::wstringstream strm;
        strm << color_str;
        strm >> std::hex >> color;

        if ( 0 == color )
        {
            color = BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE;
            LOG_ERROR << "wrong console-color: " << color_str;
        }

        set_console_color( color );
        LOG_DEBUG << "console-color: " << color;
    }
}


void ConsoleCommand::set_font_face_name( const std::wstring& name )
{
    if ( name !=  L"新宋体" && name != L"Consolas" && name != L"Lucida Console"  )
    {
        LOG_ERROR << "wrong font-face-name: " << name;
        return;
    }

    HANDLE m_handle = GetStdHandle( STD_OUTPUT_HANDLE );
    CONSOLE_FONT_INFOEX f;
    f.cbSize = sizeof( CONSOLE_FONT_INFOEX );
    GetCurrentConsoleFontEx( m_handle, FALSE, &f );
    if ( f.FaceName != name )
    {
        //SetConsoleOutputCP( face_name == L"新宋体" ? 936 : CP_UTF8 );
        SetConsoleOutputCP( name == L"新宋体" ? 936 : 936 );
        wcscpy_s( f.FaceName, name.c_str() );
        SetCurrentConsoleFontEx( m_handle, FALSE, &f );
    }
}


void ConsoleCommand::set_font_size( SHORT font_size )
{
    CONSOLE_FONT_INFOEX f;
    f.cbSize = sizeof( CONSOLE_FONT_INFOEX );
    GetCurrentConsoleFontEx( m_handle, FALSE, &f );
    if ( f.dwFontSize.Y != font_size )
    {
        f.dwFontSize.Y = font_size;
        SetCurrentConsoleFontEx( m_handle, FALSE, &f );
    }
}


void ConsoleCommand::set_console_width( SHORT width )
{
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo( m_handle, &csbi );
    COORD cur_size = { csbi.srWindow.Right - csbi.srWindow.Left + 1, csbi.srWindow.Bottom - csbi.srWindow.Top + 1 };

    if ( cur_size.X != width )
    {
        set_console_size( width, cur_size.Y );
    }
}


void ConsoleCommand::set_console_height( SHORT height )
{
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo( m_handle, &csbi );
    COORD cur_size = { csbi.srWindow.Right - csbi.srWindow.Left + 1, csbi.srWindow.Bottom - csbi.srWindow.Top + 1 };

    if ( cur_size.Y != height )
    {
        set_console_size( cur_size.X, height );
    }
}


void ConsoleCommand::set_console_size( SHORT width, SHORT height )
{
    COORD coord = { 0, 0 };
    COORD size = { width, height };
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo( m_handle, &csbi );
    COORD cur_size = { csbi.srWindow.Right - csbi.srWindow.Left + 1, csbi.srWindow.Bottom - csbi.srWindow.Top + 1 };

    if ( (size.X != cur_size.X) || (size.Y != cur_size.Y) )
    {
        COORD tmp_buf_size = { std::max<int>(cur_size.X, width), std::max<int>(cur_size.Y, height) };
        SetConsoleScreenBufferSize( m_handle, tmp_buf_size );

        SMALL_RECT window = csbi.srWindow;
        window.Right = window.Left + width - 1;
        window.Bottom = window.Top + height - 1;
        SetConsoleWindowInfo( m_handle, TRUE, &window );

        SetConsoleScreenBufferSize( m_handle, size );
    }
}


void ConsoleCommand::set_console_color( WORD color )
{
    DWORD written = 0;
    COORD coord = { 0, 0 };
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo( m_handle, &csbi );
    csbi.wAttributes = color;
    SetConsoleTextAttribute( m_handle, csbi.wAttributes );
    FillConsoleOutputAttribute( m_handle, csbi.wAttributes, csbi.dwSize.X * csbi.dwSize.Y, coord, &written );
}


void ConsoleCommand::show_console_cursor( BOOL visible )
{
    CONSOLE_CURSOR_INFO cursor_info;
    GetConsoleCursorInfo( m_handle, &cursor_info );

    if ( visible != cursor_info.bVisible )
    {
        cursor_info.bVisible = visible;
        SetConsoleCursorInfo( m_handle, &cursor_info );
    }
}


void ConsoleCommand::disable_console_system_buttons()
{
    HWND w = GetConsoleWindow();
    HMENU m = GetSystemMenu( w, FALSE );
    DeleteMenu( m, SC_CLOSE , MF_BYCOMMAND );
    DeleteMenu( m, SC_MINIMIZE , MF_BYCOMMAND );
    DeleteMenu( m, SC_MAXIMIZE , MF_BYCOMMAND );
    DrawMenuBar( w );
}
