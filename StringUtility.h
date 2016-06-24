#pragma once


namespace Utility
{
    std::vector<std::wstring> split_string( const std::wstring& s, const std::wstring& separator = L",:;/%~-\t|" );
    std::vector<std::wstring> split_string_to_lines( const std::wstring& s );
    std::vector<std::wstring> extract_strings_in_braces( const std::wstring& s, const wchar_t lc = L'{', const wchar_t rc = L'}' );

    template<typename Container, typename Any>
    void remove_if_isany_of( Container& c, Any a )
    {
        c.erase( std::remove_if( c.begin(), c.end(), boost::is_any_of( a ) ), c.end() );
    }
}
