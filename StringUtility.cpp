#include "stdafx.h"
#include "StringUtility.h"


namespace Utility
{

    std::vector<std::wstring> split_string( const std::wstring& s, const std::wstring& separator )
    {
        std::vector<std::wstring> strings;
        typedef boost::tokenizer< boost::char_separator<wchar_t>, std::wstring::const_iterator, std::wstring> tokenizer;
        boost::char_separator<wchar_t> sep( separator.c_str() );
        tokenizer tokens( s, sep );

        for ( tokenizer::iterator it = tokens.begin(); it != tokens.end(); ++it )
        {
            strings.push_back( *it );
        }

        return strings;
    }


    std::vector<std::wstring> split_string_to_lines( const std::wstring& s )
    {
        std::vector<std::wstring> vws;
        size_t size = std::count( s.begin(), s.end(), L'\n' );
        vws.reserve( size + 1 );
        size_t beg = 0;
        size_t end = s.find( L'\n', beg );
        while ( end != s.npos )
        {
            vws.push_back( s.substr( beg, end - beg ) );
            beg = end + 1;
            end = s.find( L'\n', beg );
        }

        vws.push_back( s.substr( beg ) );
        return vws;
    }


    std::vector<std::wstring> extract_strings_in_braces( const std::wstring& s, const wchar_t lc, const wchar_t rc )
    {
        std::vector<std::wstring> words;
        static boost::wregex e;

        if ( e.empty() )
        {
            std::wstringstream strm;
            strm << L"\\" << lc << L"([^" << lc << rc << L"]+)\\" << rc;
            e.assign( strm.str() );
        }

        boost::wsregex_iterator it( s.begin(), s.end(), e );
        boost::wsregex_iterator end;

        for ( ; it != end; ++it )
        {
            std::wstring w = boost::trim_copy( it->str(1) );

            if ( ! w.empty() )
            {
                words.push_back( w );
            }
        }

        return words;
    }

}
