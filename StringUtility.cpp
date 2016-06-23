#include "stdafx.h"
#include "StringUtility.h"


namespace Utility
{

    std::vector<std::string> split_string( const std::string& s, const std::string& separator )
    {
        std::vector<std::string> strings;
        typedef boost::tokenizer< boost::char_separator<char> > tokenizer;
        boost::char_separator<char> sep( separator.c_str() );
        tokenizer tokens( s, sep );

        for ( tokenizer::iterator it = tokens.begin(); it != tokens.end(); ++it )
        {
            strings.push_back( *it );
        }

        return strings;
    }


    std::vector<std::string> extract_strings_in_braces( const std::string& s, const char lc, const char rc )
    {
        std::vector<std::string> words;
        static boost::regex e;

        if ( e.empty() )
        {
            std::stringstream strm;
            strm << "\\" << lc << "([^" << lc << rc << "]+)\\" << rc;
            e.assign( strm.str() );
        }

        boost::sregex_iterator it( s.begin(), s.end(), e );
        boost::sregex_iterator end;

        for ( ; it != end; ++it )
        {
            std::string w = boost::trim_copy( it->str(1) );

            if ( ! w.empty() )
            {
                words.push_back( w );
            }
        }

        return words;
    }

}
