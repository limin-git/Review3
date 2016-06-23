#include "stdafx.h"
#include "Loader.h"
#include "Utility.h"
#include "Log.h"
#include "OptionString.h"
#include "DirectoryWatcher.h"
#include "ProgramOptions.h"



Loader::Loader( boost::function<size_t (const std::string&)> hash_function )
    : m_last_write_time( 0 ),
      m_hash_function( hash_function )
{
    m_file_name = ProgramOptions::get_vm()[file_name_option].as<std::string>();
    DirectoryWatcher::connect_to_signal( boost::bind( &Loader::process_file_change, this), m_file_name );
    reload();
}


const std::set<size_t>& Loader::get_string_hash_set()
{
    return m_string_hash_set;
}


const std::string& Loader::get_string( size_t hash )
{
    boost::unique_lock<boost::mutex> lock( m_mutex );

    std::map<size_t, std::string>::iterator it = m_hash_2_string_map.find( hash );

    if ( it != m_hash_2_string_map.end() )
    {
        return it->second;
    }

    static std::string empty( "<not-found>" );
    return empty;
}


void Loader::reload()
{
    boost::unique_lock<boost::mutex> lock( m_mutex );

    if ( ! boost::filesystem::exists( m_file_name ) )
    {
        LOG << "can not find " << m_file_name;
        m_string_hash_set.clear();
        m_hash_2_string_map.clear();
        m_last_write_time = 0;
        return;
    }

    std::time_t t = boost::filesystem::last_write_time( m_file_name );

    if ( t == m_last_write_time )
    {
        return;
    }

    LOG_DEBUG << "last-writ-time: " << Utility::string_from_time_t( m_last_write_time ) << ", new last-write-time: " << Utility::string_from_time_t( t );

    std::ifstream is( m_file_name.c_str() );

    if ( !is )
    {
        LOG << "cannot open file: " << m_file_name;
        return;
    }

    int code_page = CP_ACP;

    {
        // UTF-8: EF BB BF
        unsigned char ch[3] = { 0 };
        for ( size_t i = 0; i < 3; ++i ) { is >> ch[i]; }
        if ( ch[0] == 0xEF && ch[1] == 0xBB && ch[2] == 0xBF )
        {
            code_page = CP_UTF8;
        }
        else
        {
            for ( int i = 3; 0 < i; --i ) { is.putback( ch[i - 1] ); }
        }
    }

    std::set<size_t> string_hash_set;
    std::map<size_t, std::string> hash_2_string_map;

    for ( std::string s; std::getline( is, s ); )
    {
        boost::trim(s);
        boost::replace_all( s, "\\n", "\n" );
        boost::replace_all( s, "\\t", "\t" );

        if ( s.empty() || '#' == s[0] )
        {
            continue;
        }

        if ( code_page != CP_UTF8 )
        {
            s = Utility::to_string( s, code_page, CP_UTF8 );
        }

        size_t hash = m_hash_function( s );

        if ( hash != 0 )
        {
            string_hash_set.insert( hash );
            hash_2_string_map[hash] = s;
        }
    }

    if ( m_string_hash_set != string_hash_set )
    {
        LOG_DEBUG << "old-size = " << m_string_hash_set.size() << ", new-size = " << string_hash_set.size();

        if ( m_last_write_time != 0 )
        {
            LOG_DEBUG << get_difference( m_string_hash_set, m_hash_2_string_map, string_hash_set, hash_2_string_map );
        }

        m_string_hash_set = string_hash_set;
        m_hash_2_string_map = hash_2_string_map;
    }

    m_last_write_time = t;
}


size_t Loader::string_hash( const std::string& str )
{
    std::string s = str;
    static boost::regex e( "(?x) \\[ [a-zA-Z0-9_ -] \\]" );
    s = boost::regex_replace( s, e, "" );
    boost::to_lower(s);
    std::wstring ws = Utility::to_wstring( s, CP_UTF8 );
    const wchar_t* symbols =L" \"\',.?:;!-/#()|<>{}[]~`@$%^&*+\n\t"
        L"£¬¡£¡¢£¿£¡£»£º¡¤£®¡°¡±¡®¡¯£à£­£½¡«£À£££¤£¥£ª£ß£«£ü¡ª¡ª¡ª¡­¡­¡­¡¶¡·£¨£¨¡¾¡¿¡¸¡¹¡º¡»¡¼¡½¡´¡µ£û£ý";
    ws.erase( std::remove_if( ws.begin(), ws.end(), boost::is_any_of( symbols ) ), ws.end() );
    Utility::remove_if_isany_of(ws, symbols  );

    if ( ws.empty() )
    {
        return 0;
    }

    s = Utility::to_string( ws, CP_UTF8 );
    static boost::hash<std::string> string_hasher;
    size_t hash = string_hasher(s);
    LOG_TRACE << "hash = " << hash << " \t" << WSTRING(s);
    return hash;
}


std::string Loader::get_difference( const HashSet& os, const HashStringMap& om, const HashSet& ns, const HashStringMap& nm )
{
    std::set<size_t> removed;
    std::set_difference( os.begin(), os.end(), ns.begin(), ns.end(), std::inserter(removed, removed.begin()) );

    std::set<size_t> added;
    std::set_difference( ns.begin(), ns.end(), os.begin(), os.end(), std::inserter(added, added.begin()) );

    std::stringstream strm;

    for ( std::set<size_t>::iterator it = removed.begin(); it != removed.end(); ++it )
    {
        std::map<size_t, std::string>::const_iterator find_it = om.find( *it );

        if ( find_it != om.end() )
        {
            strm << std::endl << "remove: " << find_it->second;
        }
    }

    for ( std::set<size_t>::iterator it = added.begin(); it != added.end(); ++it )
    {
        std::map<size_t, std::string>::const_iterator find_it = nm.find( *it );

        if ( find_it != nm.end() )
        {
            strm << std::endl << "add: " << find_it->second;
        }
    }

    return strm.str();
}


void Loader::process_file_change()
{
    reload();
}
