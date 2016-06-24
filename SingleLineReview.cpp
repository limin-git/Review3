#include "stdafx.h"
#include "SingleLineReview.h"


SingleLineReview::SingleLineReview( const std::wstring& file_name, const boost::program_options::variables_map& vm )
    : m_file_name( file_name ),
      m_variable_map( vm ),
      m_is_reviewing( false )
{
    m_collect_interval = vm["collect-interval"].as<size_t>();
    m_minimal_review_time = vm["minimal-review-time"].as<size_t>();

    m_review_name   = boost::filesystem::change_extension( m_file_name, L".review" ).wstring();
    m_history_name  = boost::filesystem::change_extension( m_file_name, L".history" ).wstring();

    m_log_debug.add_attribute( "Level", boost::log::attributes::constant<std::wstring>( L"DEBUG" ) );
    m_log_trace.add_attribute( "Level", boost::log::attributes::constant<std::wstring>( L"TRACE" ) );
    m_log_test.add_attribute( "Level", boost::log::attributes::constant<std::wstring>( L"TEST" ) );

    {
        std::wstringstream strm;
        boost::chrono::seconds s;
        std::vector<std::wstring> string_list;

        if ( vm.count( "review-time-span-list" ) )
        {
            string_list = vm["review-time-span-list"].as< std::vector<std::wstring> >();
        }
        else
        {
            const wchar_t* s[] =
            {
                L"0 seconds",
                L"7 minutes",    L"30 minutes",   L"30 minutes",   L"30 minutes",   L"1 hours",      L"1 hours",      L"1 hours",
                L"1 hours",      L"2 hours",      L"3 hours",      L"4 hours",      L"5 hours",      L"6 hours",      L"7 hours",
                L"8 hours",      L"9 hours",      L"10 hours",     L"11 hours",     L"12 hours",     L"13 hours",     L"14 hours",
                L"24 hours",     L"48 hours",     L"72 hours",     L"96 hours",     L"120 hours",    L"144 hours",    L"168 hours",
                L"192 hours",    L"216 hours",    L"240 hours",    L"264 hours",    L"288 hours",    L"312 hours",    L"336 hours"
            };

            string_list.assign( s, s + sizeof(s) / sizeof(char*) );
        }

        for ( size_t i = 0; i < string_list.size(); ++i )
        {
            strm.clear();
            strm.str( string_list[i] );
            strm >> s;
            m_review_spans.push_back( s.count() );
        }

        strm.clear();
        strm.str(L"");

        //std::copy( string_list.begin(), string_list.end(), std::ostream_iterator<std::wstring>( strm, L", " ) );
        for ( size_t i = 0; i < string_list.size(); ++i )
        {
            strm << string_list[i] << L", ";
        }
        BOOST_LOG(m_log_trace) << __FUNCTION__ << " - review-time-span(" << string_list.size() << "): " << strm.str();
    }

    std::srand ( unsigned ( std::time(0) ) );
    new boost::thread( boost::bind( &SingleLineReview::collect_reviewing_strings_thread, this ) );
}


void SingleLineReview::review()
{
    initialize_history();

    for ( ;; )
    {
        reload_strings();
        synchronize_history();
        collect_reviewing_strings();
        wait_for_input( "\t***** " + boost::lexical_cast<std::wstring>(m_reviewing_strings.size() ) + " *****\n" );

        if ( ! m_reviewing_strings.empty() )
        {
            std::wstring c;
            boost::timer::cpu_timer t;

            {
                on_review_begin();

                for ( size_t i = 0; i < m_reviewing_strings.size(); ++i )
                {
                    t.start();

                    c = display_reviewing_string( m_reviewing_strings, i );

                    if ( c.empty() )
                    {
                        c = wait_for_input();
                    }

                    if ( t.elapsed().wall < m_minimal_review_time * 1000 * 1000 )
                    {
                        i--;
                        continue;
                    }

                    if ( c == "repeat" || c == "r" )
                    {
                        i--;
                        continue;
                    }

                    if ( c == "back" || c == "b" )
                    {
                        if ( 0 < i ) i -= 2;
                        else if ( 0 == i ) i--;
                        continue;
                    }
                    
                    if ( c == "skip" || c == "s" )
                    {
                        continue;
                    }

                    write_review_time( m_reviewing_strings[i] );
                }

                on_review_end();
            }
        }

        write_review_to_history();
    }
}

bool foo_bar( const string_hash_pair& lhs, const string_hash_pair& rhs )
{
    return true;
}
 

void SingleLineReview::on_review_begin()
{
    struct ReviewOrder
    {
        this_type& m_this;
        ReviewOrder( this_type& this_ ) : m_this( this_ ) {}
        bool operator()( const string_hash_pair& lhs, const string_hash_pair& rhs ) const
        {
            const time_list& ltimes = m_this.m_history[lhs.second];
            const time_list& rtimes = m_this.m_history[rhs.second];
            std::time_t lt = 0; if ( ! ltimes.empty() ) lt = ltimes.back();
            std::time_t rt = 0; if ( ! rtimes.empty() ) rt = rtimes.back();
            return 
                ( lt < rt ) ||
                ( lt == rt && lhs.first.size() < rhs.first.size() ) ||
                ( lt == rt && lhs.first.size() == rhs.first.size() && lhs.first < rhs.first )
                ;
        }
    };

    m_is_reviewing = true;
    m_review_strm.open( m_review_name.c_str(), std::ios::app );
    ReviewOrder order( *this );
    // std::random_shuffle( m_reviewing_strings.begin(), m_reviewing_strings.end(), random_gen );
    std::sort( m_reviewing_strings.begin(), m_reviewing_strings.end(), order );
    system( ( "TITLE " + m_file_name + " - " + boost::lexical_cast<std::wstring>( m_reviewing_strings.size() ) ).c_str() );
}


void SingleLineReview::on_review_end()
{
    m_is_reviewing = false;
    m_review_strm.close();
    m_reviewing_strings.clear();
    system( ( "TITLE " + m_file_name ).c_str() );
}


void SingleLineReview::initialize_history()
{
    m_history.clear();
    bool should_write_history = false;
    history_type history = load_history_from_file( m_history_name );

    merge_history( history );
    
    if ( m_history != history )
    {
        BOOST_LOG(m_log_debug) << __FUNCTION__ << " - wrong history detected.";
        should_write_history = true;
    }

    history_type review_history = load_history_from_file( m_review_name );

    if ( ! review_history.empty() )
    {
        BOOST_LOG(m_log_debug) << __FUNCTION__ << " - review detected.";
        merge_history( review_history );
        boost::filesystem::remove( m_review_name );
        should_write_history = true;
    }

    if ( should_write_history )
    {
        write_history();
    }

    BOOST_LOG(m_log_debug) << __FUNCTION__ << " - history is uptodate.\n" << get_history_string( m_history );
}


void SingleLineReview::reload_strings( hash_functor hasher )
{
    static std::time_t m_last_write_time = 0;
    std::time_t t = boost::filesystem::last_write_time( m_file_name );

    if ( t == m_last_write_time )
    {
        return;
    }

    m_last_write_time = t;

    std::ifstream is( m_file_name.c_str() );

    if ( !is )
    {
        BOOST_LOG(m_log) << __FUNCTION__ << " - cannot open file: " << m_file_name;
        return;
    }

    string_hash_list strings;

    for ( std::wstring s; std::getline( is, s ); )
    {
        boost::trim(s);

        if ( ! s.empty() )
        {
            if ( '#' == s[0] )
            {
                continue;
            }

            strings.push_back( std::make_pair( s, hasher(s) ) );
        }
    }

    if ( m_strings != strings )
    {
        m_strings.swap( strings );
        BOOST_LOG(m_log_debug) << __FUNCTION__ << " - " << "size = " << m_strings.size();
    }
}


void SingleLineReview::collect_reviewing_strings()
{
    boost::unique_lock<boost::mutex> guard( m_mutex );
    m_reviewing_strings.clear();
    
    for ( size_t i = 0; i < m_strings.size(); ++i )
    {
        size_t hash = m_strings[i].second;
        time_list& times = m_history[hash];

        if ( times.empty() )
        {
            m_reviewing_strings.push_back( m_strings[i] );
            continue;
        }

        size_t review_round = times.size();

        if ( m_review_spans.size() == review_round )
        {
            BOOST_LOG(m_log_debug) << __FUNCTION__ << " - finished (" << hash << "): " << m_strings[i].first;
            continue;
        }

        std::time_t current_time = std::time(0);
        std::time_t last_review_time = times.back();
        std::time_t span = m_review_spans[review_round];

        if ( last_review_time + span < current_time )
        {
            m_reviewing_strings.push_back( m_strings[i] );
        }

        if ( current_time < last_review_time )
        {
            BOOST_LOG(m_log) << __FUNCTION__ << " - wrong time: last-review-time=" << time_string( last_review_time );
        }

        BOOST_LOG(m_log_trace) << __FUNCTION__ << " -"
            << " first-review-time = " << time_string( times.front() )
            << " last-review-time = " << time_string(last_review_time)
            << " review-round = " << review_round
            << " span = " << time_duration_string( span )
            << " elapsed = " << time_duration_string( current_time - last_review_time )
            ;
    }
}


void SingleLineReview::collect_reviewing_strings_thread()
{
    while ( true )
    {
        boost::this_thread::sleep_for( boost::chrono::seconds( m_collect_interval ) );

        if ( false == m_is_reviewing )
        {
            collect_reviewing_strings();

            if ( ! m_reviewing_strings.empty() )
            {
                system( ( "TITLE " + m_file_name + " - " + boost::lexical_cast<std::wstring>( m_reviewing_strings.size() ) ).c_str() ); 
            }
        }
    }
}


void SingleLineReview::write_review_time( const string_hash_pair& s )
{
    if ( ! m_review_strm )
    {
        BOOST_LOG(m_log) << __FUNCTION__ << " - cannot open for append: " << m_review_name;
        return;
    }

    m_review_strm << s.second << "\t" << std::time(0) << std::endl;

    if ( m_review_strm.fail() )
    {
        BOOST_LOG(m_log) << __FUNCTION__ << " - failed.";
    }
}


std::wstring SingleLineReview::wait_for_input( const std::wstring& message )
{
    if ( ! message.empty() )
    {
        std::cout << message << std::flush;
    }

    std::wstring input;
    std::getline( std::cin, input );
    system( "CLS" );
    boost::trim(input);
    return input;
}


void SingleLineReview::write_review_to_history()
{
    history_type review_history = load_history_from_file( m_review_name );

    if ( ! review_history.empty() )
    {
        merge_history( review_history );

        if ( write_history() )
        {
            boost::filesystem::remove( m_review_name );
        }
    }
}


void SingleLineReview::synchronize_history()
{
    bool history_changed = false;
    std::set<size_t> hashes;

    for ( size_t i = 0; i < m_strings.size(); ++i )
    {
        hashes.insert( m_strings[i].second );
    }

    for ( history_type::iterator it = m_history.begin(); it != m_history.end(); NULL )
    {
        if ( hashes.find( it->first ) == hashes.end() )
        {
            BOOST_LOG(m_log_debug) << __FUNCTION__ << " - " << "erase: " << it->first << " " << get_time_list_string( it->second );
            m_history.erase( it++ );
            history_changed = true;
        }
        else
        {
            ++it;
        }
    }

    if ( history_changed )
    {
        write_history();
    }
}


std::wstring SingleLineReview::display_reviewing_string( const string_hash_list& strings, size_t index )
{
    std::stringstream strm;
    strm << "TITLE " << m_file_name << " " << index + 1 << " / " << strings.size();
    system( strm.str().c_str() );

    const std::wstring& s = strings[index].first;

    if ( boost::starts_with( s, "[Q]" ) || boost::starts_with( s, "[q]" ) )
    {
        size_t pos = s.find( "[A]" );

        if ( pos == std::wstring::npos )
        {
            pos = s.find( "[a]" );

            if ( pos == std::wstring::npos )
            {
                BOOST_LOG(m_log) << __FUNCTION__ << " - bad format: " << s;
                return "";
            }
        }

        std::wstring question = s.substr( 4, pos - 4 );
        std::wstring answer = s.substr( pos + 4 );
        boost::trim(question);
        boost::trim(answer);

        if ( !question.empty() && ! answer.empty() )
        {
            std::cout << "\t" << question << std::flush;

            std::wstring command;
            std::getline( std::cin, command );
            if ( ! command.empty() )
            {
                return command;
            }

            std::cout << "\t" << answer << std::flush;
        }
    }
    else
    {
        std::cout << "\t" << strings[index].first << std::flush;
    }

    return "";
}


std::wstring SingleLineReview::time_string( std::time_t t, const char* format )
{
    std::tm* m = std::localtime( &t );
    char s[100] = { 0 };
    std::strftime( s, 100, format, m );
    return s;
}


std::wstring SingleLineReview::time_duration_string( std::time_t t )
{
    std::stringstream strm;
    std::time_t mon = 0, d = 0, h = 0, min = 0;

    #define CALCULATE( n, u, x ) if ( u <= x  ) { n = x / u; x %= u; }
    CALCULATE( mon, month, t );
    CALCULATE( d, day, t );
    CALCULATE( h, hour, t );
    CALCULATE( min, minute, t );
    #undef CALCULATE

    #define WRAP_ZERO(x) (9 < x ? "" : "0") << x 
    if ( mon || d ) { strm << WRAP_ZERO(mon) << "/" << WRAP_ZERO(d) << "-"; }
    strm << WRAP_ZERO(h) << ":" << WRAP_ZERO(min);
    #undef WRAP_ZERO

    return strm.str();
}


bool SingleLineReview::write_history()
{
    std::ofstream os( m_history_name.c_str() );

    if ( ! os )
    {
        BOOST_LOG(m_log) << __FUNCTION__ << " - can not open file for write " << m_history_name;
        return false;
    }

    for ( history_type::const_iterator it = m_history.begin(); it != m_history.end(); ++it )
    {
        os << it->first << " ";
        std::copy( it->second.begin(), it->second.end(), std::ostream_iterator<std::time_t>(os, " ") );
        os << std::endl;
    }

    BOOST_LOG(m_log_debug) << __FUNCTION__ << " - " << "update history, size = " << m_history.size();
    return true;
}


history_type SingleLineReview::load_history_from_file( const std::wstring& file_name )
{
    history_type history;

    if ( ! boost::filesystem::exists( file_name ) )
    {
        return history;
    }

    std::ifstream is( file_name.c_str() );

    if ( ! is )
    {
        return history;
    }

    size_t hash = 0;
    std::time_t time = 0;
    std::stringstream strm;

    for ( std::wstring s; std::getline( is, s ); )
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

    return history;
}


void SingleLineReview::merge_history( const history_type& history )
{
    for ( history_type::const_iterator it = history.begin(); it != history.end(); ++it )
    {
        size_t hash = it->first;
        const std::vector<std::time_t>& times = it->second;
        std::vector<std::time_t>& history_times = m_history[hash];
        size_t round = history_times.size();
        std::time_t last_time = ( round ? history_times.back() : 0 );

        for ( size_t i = 0; i < times.size(); ++i )
        {
            if ( last_time + m_review_spans[round] < times[i] )
            {
                history_times.push_back( times[i] );
                last_time = times[i];
                round++;
            }
        }
    }
}


std::ostream& SingleLineReview::output_history( std::ostream& os, const history_type& history )
{
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

        os << " " << time_string( times[0] );

        for ( size_t i = 1; i < times.size(); ++i )
        {
            os << " " << time_duration_string( times[i] - times[i - 1] );
        }

        os << std::endl;
    }

    return os;
}


std::wstring SingleLineReview::get_history_string( const history_type& history )
{
    std::stringstream strm;
    output_history( strm, history );
    return strm.str();
}


std::ostream& SingleLineReview::output_time_list( std::ostream& os, const time_list& times )
{
    if ( times.empty() )
    {
        return os;
    }

    os << " " << time_string( times[0] );

    for ( size_t i = 1; i < times.size(); ++i )
    {
        os << " " << time_duration_string( times[i] - times[i - 1] );
    }

    return os;
}

std::wstring SingleLineReview::get_time_list_string( const time_list& times )
{
    std::stringstream strm;
    output_time_list( strm, times );
    return strm.str();
}


size_t SingleLineReview::string_hash( const std::wstring& str )
{
    std::wstring s = str;
    const char* chinese_chars[] =
    {
        "¡¡", "£¬", "¡£", "¡¢", "£¿", "£¡", "£»", "£º", "¡¤", "£®", "¡°", "¡±", "¡®", "¡¯",
        "£à", "£­", "£½", "¡«", "£À", "££", "£¤", "£¥", "£ª", "£ß", "£«", "£ü", "¡ª", "¡ª¡ª",  "¡­", "¡­¡­",
        "¡¶", "¡·", "£¨", "£¨", "¡¾", "¡¿", "¡¸", "¡¹", "¡º", "¡»", "¡¼", "¡½", "¡´", "¡µ", "£û", "£ý",
    };

    for ( size_t i = 0; i < sizeof(chinese_chars) / sizeof(char*); ++i )
    {
        boost::erase_all( s, chinese_chars[i] );
    }

    s.erase( std::remove_if( s.begin(), s.end(), boost::is_any_of( " \t\"\',.?:;!-/#()|<>{}[]~`@$%^&*+" ) ), s.end() );
    boost::to_lower(s);
    static boost::hash<std::wstring> string_hasher;
    return string_hasher(s);
}


void SingleLineReview::update_hash_algorighom( hash_functor old_hasher, hash_functor new_hasher )
{
    initialize_history();
    reload_strings( old_hasher );

    history_type history;

    for ( size_t i = 0; i < m_strings.size(); ++i )
    {
        std::wstring s = m_strings[i].first;
        size_t hash = m_strings[i].second;
        time_list& times = m_history[hash];

        size_t new_hash = new_hasher(s);

        m_strings[i].second = new_hash;
        history[new_hash] = times;
    }

    if ( history.size() == m_history.size() )
    {
        m_history = history;
        write_history();
        BOOST_LOG(m_log_debug) << __FUNCTION__ << " - history updated";
    }
}


bool SingleLineReview::view_order( const string_hash_pair& lhs, const string_hash_pair& rhs ) const
{
    return true;
}
