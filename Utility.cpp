#include "stdafx.h"
#include "Utility.h"


namespace Utility
{

    size_t random_number( size_t lo, size_t hi )
    {
        static boost::random::mt19937 gen( static_cast<boost::uint32_t>( std::time(0) ) );
        static boost::random::uniform_int_distribution<> dist;
        size_t x = dist( gen );

        if ( lo != std::numeric_limits <size_t> ::min() || hi != std::numeric_limits <size_t> ::max() )
        {
            while ( x < lo || hi < x )
            {
                x %= hi;
                x += lo;
            }
        }

        return x;
    }


    void set_system_wallpaper( const std::wstring& picture )
    {
        SystemParametersInfo(SPI_SETDESKWALLPAPER, 0, const_cast<wchar_t*>( picture.c_str() ), SPIF_UPDATEINIFILE);
    }


    std::vector<std::wstring> get_files_of_directory( const std::wstring& dir )
    {
        std::vector<std::wstring> files;
        boost::filesystem::path dir_path( dir );

        if ( !exists( dir_path ) )
        {
            return files;
        }

        boost::filesystem::recursive_directory_iterator end;
        for ( boost::filesystem::recursive_directory_iterator it( dir_path ); it != end; ++it )
        {
            if ( false != is_directory( it->status() ) )
            {
                files.push_back( it->path().wstring() );
            }
        }

        return files;
    }


    bool is_picture( const std::wstring& file_name )
    {
        std::ifstream is( file_name.c_str(), std::ios::in | std::ios::binary );

        if ( !is )
        {
            return false;
        }

        unsigned char ch[2] = { 0 };

        is.unsetf( std::ios::skipws );
        is.read( (char*)ch, 2 );

        if ( ch[0] == 0xFF && ch[1] == 0xD8 ) // JPEG
        {
            return true;
        }
        else if ( ch[0] == 0x4D && ch[1] == 0x42 ) // BMP
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    bool remove_file( const std::wstring& file_name )
    {
        boost::filesystem::path p( file_name );

        if ( exists( p ) )
        {
            boost::system::error_code ec;
            permissions(p, boost::filesystem::all_all);
            remove( p, ec);

            if ( ec )
            {
                return false;
            }
        }

        return true;
    }

}
