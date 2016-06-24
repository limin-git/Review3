#include "stdafx.h"
#include "Loader.h"
#include "Utility.h"
#include "Log.h"
#include "OptionString.h"
#include "DirectoryWatcher.h"
#include "ProgramOptions.h"


Loader::Loader( boost::function<size_t (const std::wstring&)> hash_function )
    : m_last_write_time( 0 ),
      m_hash_function( hash_function )
{
    m_file_name = ProgramOptions::get_vm()[file_name_option].as<std::wstring>();
    DirectoryWatcher::connect_to_signal( boost::bind( &Loader::process_file_change, this), m_file_name );
    reload();
}


const std::set<size_t>& Loader::get_string_hash_set()
{
    return m_string_hash_set;
}


const std::wstring& Loader::get_string( size_t hash )
{
    boost::unique_lock<boost::mutex> lock( m_mutex );

    std::map<size_t, std::wstring>::iterator it = m_hash_2_string_map.find( hash );

    if ( it != m_hash_2_string_map.end() )
    {
        return it->second;
    }

    static std::wstring empty( L"<.N.A.>" );
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

    std::wstring fise_string = Utility::wstring_from_file( m_file_name.c_str() );
    std::vector<std::wstring> lines = Utility::split_string_to_lines( fise_string );

    if ( lines.empty() )
    {
        LOG << "cannot open file: " << m_file_name;
        return;
    }

    std::set<size_t> string_hash_set;
    std::map<size_t, std::wstring> hash_2_string_map;

    for ( size_t i = 0; i < lines.size(); ++i )
    {
        std::wstring& s = lines[i];

        boost::trim(s);
        boost::replace_all( s, L"\\n", L"\n" );
        boost::replace_all( s, L"\\t", L"\t" );

        if ( s.empty() || L'#' == s[0] )
        {
            continue;
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


size_t Loader::string_hash( const std::wstring& str )
{
    std::wstring s = str;
    static boost::wregex e( L"(?x) \\[ [a-zA-Z0-9_ -] \\]" );
    s = boost::regex_replace( s, e, L"" );
    boost::to_lower(s);
    const wchar_t* symbols =L" \"\',.?:;!-/#()|<>{}[]~`@$%^&*+\n\t"
        L"£¬¡£¡¢£¿£¡£»£º¡¤£®¡°¡±¡®¡¯£à£­£½¡«£À£££¤£¥£ª£ß£«£ü¡ª¡ª¡ª¡­¡­¡­¡¶¡·£¨£¨¡¾¡¿¡¸¡¹¡º¡»¡¼¡½¡´¡µ£û£ý";
    Utility::remove_if_isany_of(s, symbols  );

    if ( s.empty() )
    {
        return 0;
    }

    static boost::hash<std::wstring> string_hasher;
    size_t hash = string_hasher(s);
    LOG_TRACE << "hash = " << hash << " \t" << s;
    return hash;
}


std::wstring Loader::get_difference( const HashSet& os, const HashStringMap& om, const HashSet& ns, const HashStringMap& nm )
{
    std::set<size_t> removed;
    std::set_difference( os.begin(), os.end(), ns.begin(), ns.end(), std::inserter(removed, removed.begin()) );

    std::set<size_t> added;
    std::set_difference( ns.begin(), ns.end(), os.begin(), os.end(), std::inserter(added, added.begin()) );

    std::wstringstream strm;

    for ( std::set<size_t>::iterator it = removed.begin(); it != removed.end(); ++it )
    {
        std::map<size_t, std::wstring>::const_iterator find_it = om.find( *it );

        if ( find_it != om.end() )
        {
            strm << std::endl << L"remove: " << find_it->second;
        }
    }

    for ( std::set<size_t>::iterator it = added.begin(); it != added.end(); ++it )
    {
        std::map<size_t, std::wstring>::const_iterator find_it = nm.find( *it );

        if ( find_it != nm.end() )
        {
            strm << std::endl << L"add: " << find_it->second;
        }
    }

    return strm.str();
}


void Loader::process_file_change()
{
    reload();
}
