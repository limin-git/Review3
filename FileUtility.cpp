#include "stdafx.h"
#include "FileUtility.h"


namespace Utility
{
    std::wstring wstring_from_file( const wchar_t* file_name, int code_page )
    {
        std::wstring ws;
        std::ifstream is( file_name, std::ios::in | std::ios::binary );

        if ( !is )
        {
            return L"";
        }

        unsigned char ch[3] = { 0 };

        is.unsetf( std::ios::skipws );
        is.read( (char*)ch, 3 );

        if ( ch[0] == 0xFF && ch[1] == 0xFE ) // unicode: FF FE
        {
            is.seekg( -2, std::ios::end );
            size_t size = is.tellg();
            ws.resize( size / 2 );
            is.seekg( 2, std::ios::beg );
            is.read( (char*)&ws[0], size );
        }
        else if ( ch[0] == 0xEF && ch[1] == 0xBB && ch[2] == 0xBF ) // utf-8 + BOM: EF BB BF
        {
            is.seekg( -3, std::ios::end );
            size_t size = is.tellg();
            char* buf = new char[ size ];
            is.seekg( 3, std::ios::beg );
            is.read( buf, size );
            size_t wsize = MultiByteToWideChar( CP_UTF8, 0, buf, size, 0, 0 );
            ws.resize( wsize );
            MultiByteToWideChar( CP_UTF8, 0, buf, size, &ws[0], wsize );
            delete[] buf;
        }
        else // ANSI
        {
            is.seekg( 0, std::ios::end );
            size_t size = is.tellg();
            char* buf = new char[ size ];
            is.seekg( 0, std::ios::beg );
            is.read( buf, size );
            size_t wsize = MultiByteToWideChar( code_page, 0, buf, size, 0, 0 );
            ws.resize( wsize );
            MultiByteToWideChar( code_page, 0, buf, size, &ws[0], wsize );
        }

        return ws;
    }


    std::string string_from_file( const wchar_t* file_name, int file_code_page, int string_code_page )
    {
        std::string s;
        std::ifstream is( file_name, std::ios::in | std::ios::binary );

        if ( !is )
        {
            return "";
        }

        unsigned char ch[3] = { 0 };

        is.unsetf( std::ios::skipws );
        is.read( (char*)ch, 3 );

        if ( ch[0] == 0xFF && ch[1] == 0xFE ) // unicode: FF FE
        {
            is.seekg( -2, std::ios::end );
            size_t wsize = is.tellg();
            wchar_t* buf = new wchar_t[wsize/2];
            is.seekg( 2, std::ios::beg );
            is.read( (char*)buf, wsize );
            size_t size = WideCharToMultiByte( CP_UTF8, 0, buf, wsize, 0, 0, 0, 0 );
            s.resize( size );
            WideCharToMultiByte( CP_UTF8, 0, buf, wsize, &s[0], size, 0, 0 );
            delete[] buf;
        }
        else if ( ch[0] == 0xEF && ch[1] == 0xBB && ch[2] == 0xBF ) // utf-8 + BOM: EF BB BF
        {
            is.seekg( -3, std::ios::end );
            size_t size = is.tellg();
            is.seekg( 3, std::ios::beg );

            if ( string_code_page == CP_UTF8 )
            {
                s.resize( size );
                is.read( &s[0], size );
            }
            else
            {
                char* buf = new char[size];
                is.read( buf, size );
                size_t wsize = MultiByteToWideChar( CP_UTF8, 0, buf, size, 0, 0 );
                wchar_t* wbuf = new wchar_t[wsize];
                MultiByteToWideChar( CP_UTF8, 0, buf, size, wbuf, wsize );
                size_t size = WideCharToMultiByte( string_code_page, 0, wbuf, wsize, 0, 0, 0, 0 );
                s.resize( size );
                WideCharToMultiByte( string_code_page, 0, wbuf, wsize, &s[0], size, 0, 0 );
                delete[] buf;
                delete[] wbuf;
            }
        }
        else // ANSI
        {
            is.seekg( 0, std::ios::end );
            size_t size = is.tellg();
            is.seekg( 0, std::ios::beg );

            if ( file_code_page == string_code_page )
            {
                s.resize( size );
                is.read( &s[0], size );
            }
            else
            {
                char* buf = new char[size];
                is.read( buf, size );
                size_t wsize = MultiByteToWideChar( file_code_page, 0, buf, size, 0, 0 );
                wchar_t* wbuf = new wchar_t[wsize];
                MultiByteToWideChar( file_code_page, 0, buf, size, wbuf, wsize );
                size_t size = WideCharToMultiByte( string_code_page, 0, wbuf, wsize, 0, 0, 0, 0 );
                s.resize( size );
                WideCharToMultiByte( string_code_page, 0, wbuf, wsize, &s[0], size, 0, 0 );
                delete[] buf;
                delete[] wbuf;
            }
        }

        return s;
    }


}
