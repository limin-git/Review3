#include "stdafx.h"
#include "History.h"
#include "Utility.h"
#include "Log.h"
#include "OptionString.h"
#include "ProgramOptions.h"
#include "OptionUpdateHelper.h"


History::History()
    : m_max_cache_size( 100 ),
      m_cache_size( 0 ),
      m_once_per_days( 0 )
{
    const boost::program_options::variables_map& vm = ProgramOptions::get_vm();

    {
        std::wstring schedule;

        if ( vm.count( review_schedule ) )
        {
            schedule = vm[review_schedule].as<std::wstring>();
        }

        if ( schedule.empty() )
        {
            schedule = L"0 hours, 24 hours, 48 hours, 72 hours, 96 hours, 120 hours, 144 hours, 168 hours";
            //"0 seconds",    "7 minutes",    "30 minutes",   "30 minutes",   "1 hours",      "3 hours",      "5 hours",
            //"7 hours",      "9 hours",      "11 hours",     "13 hours",     "15 hours",     "17 hours",     "19 hours",
            //"24 hours",     "48 hours",     "72 hours",     "96 hours",     "120 hours",    "144 hours",    "168 hours"
            //"192 hours",    "216 hours",    "240 hours",    "264 hours",    "288 hours",    "312 hours",    "336 hours",
            //"360 hours",    "384 hours",    "408 hours",    "432 hours",    "456 hours",    "480 hours",    "504 hours",
            //"528 hours",    "552 hours",    "576 hours",    "600 hours",    "624 hours",    "648 hours",    "672 hours",
            //"696 hours",    "720 hours",    "744 hours",    "768 hours",    "792 hours",    "816 hours",    "840 hours"
        }

        std::vector<std::wstring> strings = Utility::split_string( schedule );
        m_schedule = Utility::times_from_strings( strings );
        LOG_TRACE << "review.schedule(" << strings.size() << "): " << schedule;
    }

    m_connection = ProgramOptions::connect_to_signal( boost::bind( &History::update_option, this, _1 ) );
}


History::~History()
{
    m_connection.disconnect();
    write_history();
    clean_review_cache();
}


void History::initialize()
{
    bool should_write_history = false;
    history_type history = load_history_from_file( m_file_name );

    merge_history( history );

    if ( m_history != history )
    {
        LOG_DEBUG << "wrong history detected.";
        should_write_history = true;
    }

    history_type review_history = load_history_from_file( m_review_name );

    if ( ! review_history.empty() )
    {
        LOG_DEBUG << "review detected.";
        merge_history( review_history );
        boost::filesystem::remove( m_review_name );
        should_write_history = true;
    }

    if ( should_write_history )
    {
        write_history();
    }

    LOG_TRACE << "history is updated.\n" << string_from_history( m_history );
}


void History::save_history( size_t hash, std::time_t current_time )
{
    if ( current_time == 0 )
    {
        m_history[hash].clear();
    }

    m_history[hash].push_back( current_time );

    if ( m_cache_size == 0 )
    {
        m_review_stream.open( m_review_name.c_str(), std::ios::app );

        if ( ! m_review_stream )
        {
            LOG << "cannot open for append: " << m_review_name;
            return;
        }

        LOG_DEBUG << "created a file for cache: " << m_review_name;
    }

    m_review_stream << hash << "\t" << current_time << std::endl;

    if ( m_review_stream.fail() )
    {
        m_review_stream.clear();
        LOG << "failed.";
    }

    m_cache_size++;
    LOG_DEBUG << "cache-size = " << m_cache_size;

    if ( m_max_cache_size <= m_cache_size )
    {
        write_history();
        clean_review_cache();
    }
}


void History::write_history()
{
    std::ofstream os( m_file_name.c_str() );

    if ( ! os )
    {
        LOG << "can not open file for write " << m_file_name;
        return;
    }

    for ( history_type::const_iterator it = m_history.begin(); it != m_history.end(); ++it )
    {
        if ( ! it->second.empty() )
        {
            os << it->first << " ";
            std::copy( it->second.begin(), it->second.end(), std::ostream_iterator<std::time_t>(os, " ") );
            os << std::endl;
        }
    }

    LOG_DEBUG << "update history, size = " << m_history.size();
}


history_type History::load_history_from_file( const std::wstring& file )
{
    history_type history;

    if ( ! boost::filesystem::exists( file ) )
    {
        return history;
    }

    std::ifstream is( file.c_str() );

    if ( ! is )
    {
        return history;
    }

    size_t hash = 0;
    std::time_t time = 0;
    std::stringstream strm;

    for ( std::string s; std::getline( is, s ); )
    {
        if ( ! s.empty() )
        {
            strm.clear();
            strm.str(s);
            strm >> hash;
            std::vector<std::time_t>& times = history[hash];

            while ( strm >> time )
            {
                times.push_back( time );
            }
        }
    }

    LOG_TRACE << file << std::endl << string_from_history( history );
    return history;
}


void History::merge_history( const history_type& history )
{
    for ( history_type::const_iterator it = history.begin(); it != history.end(); ++it )
    {
        size_t hash = it->first;
        const std::vector<std::time_t>& times = it->second;
        std::vector<std::time_t>& history_times = m_history[hash];
        size_t round = history_times.size();
        std::time_t last_time = ( round ? history_times.back() : 0 );

        if ( last_time == DELETED && round == 1 )
        {
            continue;
        }

        if ( last_time == FINISHED && round == 1 ) // finished
        {
            continue;
        }

        if ( m_schedule.size() <= history_times.size() ) // finished
        {
            history_times.clear();
            history_times.push_back( FINISHED );
            continue;
        }

        for ( size_t i = 0; i < times.size(); ++i )
        {
            if ( times[i] == DELETED ) // deleted
            {
                history_times.clear();
                history_times.push_back( DELETED );
                break;
            }

            if ( last_time + m_schedule[round] < times[i] )
            {
                history_times.push_back( times[i] );
                last_time = times[i];
                round++;

                if ( m_schedule.size() <= round )
                {
                    history_times.clear();
                    history_times.push_back( FINISHED );
                    break;
                }
            }
            else if ( last_time < times[i] ) // schedule changed
            {
                history_times.back() = times[i];
            }
            else
            {
                LOG_DEBUG
                    << " ignore review time: " << Utility::string_from_time_t( times[i] )
                    << " round = " << round
                    << " last-review-time = " << last_time
                    << " elapsed = " << Utility::duration_string_from_seconds( times[i] - last_time )
                    << " span = " << Utility::duration_string_from_seconds( m_schedule[round] )
                    ;
            }
        }
    }
}


void History::synchronize_history( const std::set<size_t>& hashes )
{
    bool history_changed = false;

    for ( history_type::iterator it = m_history.begin(); it != m_history.end(); NULL )
    {
        if ( hashes.find( it->first ) == hashes.end() )
        {
            LOG_DEBUG << "erase: " << it->first << " " << Utility::duration_string_from_time_list( it->second );
            m_history.erase( it++ );
            history_changed = true;
        }
        else
        {
            ++it;
        }
    }

    for ( std::set<size_t>::const_iterator it = hashes.begin(); it != hashes.end(); ++it )
    {
        if ( m_history.find( *it ) == m_history.end() )
        {
            m_history.insert( history_type::value_type( *it, time_list() ) );
            history_changed = true;
            //LOG_DEBUG << "add: " << *it;
        }
    }

    if ( history_changed )
    {
        write_history();
        clean_review_cache();
    }
}


bool History::is_expired( size_t hash, const std::time_t& current_time )
{
    time_list& times = m_history[hash];
    size_t review_round = times.size();

    if ( 0 == review_round ) // new added
    {
        return true;
    }

    std::time_t last_review_time = times.back();

    if ( last_review_time == DELETED )
    {
        return false;
    }

    if ( last_review_time == FINISHED )
    {
        return false;
    }

    if ( m_schedule.size() <= review_round ) // finished
    {
        times.clear();
        times.push_back( 1 );
        return false;
    }

    std::time_t span = m_schedule[review_round];

    if ( ! ( last_review_time + span < current_time ) )
    {
        return false;
    }

    if ( m_once_per_days )
    {
        if ( current_time - last_review_time < m_once_per_days * 24 * 3600 )
        {
            return false;
        }
    }

    return true;
}


bool History::is_not_reviewable( size_t hash )
{
    history_type::iterator it = m_history.find( hash );

    if ( it == m_history.end() )
    {
        return false;
    }

    return ( ( 1 == it->second.size() ) && ( DELETED == it->second.front() || FINISHED == it->second.front() ) );
}


std::set<size_t> History::get_expired()
{
    std::set<size_t> expired;
    std::time_t current_time = std::time(0);

    for ( history_type::iterator it = m_history.begin(); it != m_history.end(); ++it )
    {
        if ( is_expired( it->first, current_time ) )
        {
            expired.insert( it->first );
        }
    }

    return expired;
}


bool History::is_finished()
{
    for ( history_type::iterator it = m_history.begin(); it != m_history.end(); ++it )
    {
        if ( it->second.empty() )
        {
            return false;
        }

        time_t first_time = it->second.front();

        if ( first_time != DELETED && first_time != FINISHED )
        {
            return false;
        }
    }

    return true;
}


void History::clean_review_cache()
{
    if ( m_review_stream.is_open() )
    {
        m_review_stream.close();
    }

    if ( boost::filesystem::exists( m_review_name ) )
    {
        boost::filesystem::remove( m_review_name );
        LOG_DEBUG << "remove file: " << m_review_name;
    }

    m_cache_size = 0;
}


void History::update_option( const boost::program_options::variables_map& vm )
{
    static OptionUpdateHelper option_helper;
    static std::wstring name = vm[file_name_option].as<std::wstring>();
    static std::wstring default_history_naame = boost::filesystem::change_extension( name, L".history" ).wstring();
    static std::wstring default_review_naame = boost::filesystem::change_extension( name, L".review" ).wstring();

    if ( option_helper.update_one_option<std::wstring>( file_history_option, vm, default_history_naame ) )
    {
        m_file_name = option_helper.get_value<std::wstring>( file_history_option );
        LOG_DEBUG << "file-history-name: " << m_file_name;
    }

    if ( option_helper.update_one_option<std::wstring>( file_review_option, vm, default_review_naame ) )
    {
        m_review_name = option_helper.get_value<std::wstring>( file_review_option );
        LOG_DEBUG << "file-review-name: " << m_review_name;
    }

    if ( option_helper.update_one_option<size_t>( review_max_cache_size_option, vm, 100 ) )
    {
        m_max_cache_size = option_helper.get_value<size_t>( review_max_cache_size_option );
        LOG_DEBUG << "review-max-cache-size: " << m_max_cache_size;
    }

    if ( option_helper.update_one_option<size_t>( review_once_per_days_option, vm, 0 ) )
    {
        m_once_per_days = option_helper.get_value<size_t>( review_once_per_days_option );
        LOG_DEBUG << "review-once-per-days: " << m_once_per_days;
        m_once_per_days *= 3600 * 24;
    }
}


std::wstring History::string_from_history( const history_type& history )
{
    std::wstringstream os;

    for ( history_type::const_iterator it = history.begin(); it != history.end(); ++it )
    {
        size_t hash = it->first;
        const time_list& times = it->second;
        os << hash;

        if ( times.empty() )
        {
            os << std::endl;
            continue;
        }

        os << L" " << Utility::string_from_time_t( times[0] );

        if ( 1 < times.size() )
        {
            os << Utility::duration_string_from_time_list( times );
        }

        os << std::endl;
    }

    return os.str();
}
