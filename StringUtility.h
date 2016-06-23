#pragma once


namespace Utility
{
    std::vector<std::string> split_string( const std::string& s, const std::string& separator = ",:;/%~-\t|" );
    std::vector<std::string> extract_strings_in_braces( const std::string& s, const char lc = '{', const char rc = '}' );

    template<typename Container, typename Any>
    void remove_if_isany_of( Container& c, Any a )
    {
        c.erase( std::remove_if( c.begin(), c.end(), boost::is_any_of( a ) ), c.end() );
    }
}
