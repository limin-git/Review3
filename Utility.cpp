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


    void set_system_wallpaper( const std::string& picture )
    {
        SystemParametersInfoA(SPI_SETDESKWALLPAPER, 0, const_cast<char*>( picture.c_str() ), SPIF_UPDATEINIFILE);
    }


    std::vector<std::string> get_files_of_directory( const std::string& dir )
    {
        std::vector<std::string> files;
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
                files.push_back( it->path().string() );
            }
        }

        return files;
    }

}
