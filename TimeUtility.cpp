#include "stdafx.h"
#include "TimeUtility.h"


namespace Utility
{

    std::string string_from_time_t( std::time_t t, const char* format )
    {
        std::tm* m = std::localtime( &t );
        char s[100] = { 0 };
        std::strftime( s, 100, format, m );
        return s;
    }


    std::string duration_string_from_seconds( std::time_t t )
    {
        enum { minute = 60, hour = 60 * minute ,day = 24 * hour, month = 30 * day };

        std::stringstream strm;
        std::time_t mon = 0, d = 0, h = 0, min = 0;

        #define CALCULATE( x, u, n ) if ( u <= x  ) { n = x / u; x %= u; }
        CALCULATE( t, month, mon );
        CALCULATE( t, day, d );
        CALCULATE( t, hour, h );
        CALCULATE( t, minute, min );
        #undef CALCULATE

        #define WRAP_ZERO(x) (9 < x ? "" : "0") << x
        if ( mon || d ) { strm << WRAP_ZERO(mon) << "/" << WRAP_ZERO(d) << "-"; }
        strm << WRAP_ZERO(h) << ":" << WRAP_ZERO(min);
        #undef WRAP_ZERO

        return strm.str();
    }


    std::string duration_string_from_time_list( const std::vector<time_t>& times )
    {
        if ( times.empty() )
        {
            return "";
        }

        std::stringstream strm;
        strm << string_from_time_t( times[0] );

        for ( size_t i = 1; i < times.size(); ++i )
        {
            strm << ", " << duration_string_from_seconds( times[i] - times[i - 1] );
        }

        return strm.str();
    }


    std::vector<time_t> times_from_strings( const std::vector<std::string>& strings )
    {
        std::vector<time_t> times;
        boost::chrono::seconds s;
        std::stringstream strm;

        for ( size_t i = 0; i < strings.size(); ++i )
        {
            strm.clear();
            strm.str( strings[i] );
            strm >> s;

            if ( strm.fail() )
            {
                return std::vector<time_t>();
            }

            times.push_back( s.count() );
        }

        return times;
    }


}
