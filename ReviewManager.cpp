#include "stdafx.h"
#include "ReviewManager.h"
#include "History.h"
#include "Loader.h"
#include "Speech.h"
#include "ReviewString.h"
#include "Utility.h"
#include "Log.h"
#include "OptionString.h"
#include "ProgramOptions.h"
#include "OptionUpdateHelper.h"


struct Order
{
    Order( Loader* l, History* h ) : loader(l), history(h) {}

    bool operator()( size_t lhs, size_t rhs ) const
    {
        size_t lr = history->get_review_round(lhs);
        size_t rr = history->get_review_round(rhs);
        std::time_t lt = history->get_last_review_time( lhs );
        std::time_t rt = history->get_last_review_time( rhs );
        return ( ( lr < rr ) || ( lr == rr && rt < lt ) || ( lr == rr && lt == rt && lhs < rhs ) );
    }

    Loader* loader;
    History* history;
};


ReviewManager::ReviewManager()
    : m_review_mode( Forward ),
      m_backward_index( 0 ),
      m_loader( NULL ),
      m_history( NULL ),
      m_speech( NULL ),
      m_speech_impl( NULL ),
      m_is_listening( false ),
      m_minimal_review_time( 500 * 1000 * 1000 ),
      m_auto_update_interval( 60 ),
      m_play_back( 0 ),
      m_current_reviewing( NULL ),
      m_listen_no_string( false ),
      m_listen_all( false ),
      m_review_number( 0 ),
      m_minimal_review_distance( 10 ),
      m_running( true )
{
    m_loader = new Loader;
    m_history = new History;
    m_speech_impl = new Speech;
    m_connection = ProgramOptions::connect_to_signal( boost::bind( &ReviewManager::update_option, this, _1 ) );
}


ReviewManager::~ReviewManager()
{
    show_next_picture( "C:\\Windows\\Web\\Wallpaper\\Theme1\\img1.jpg" );
    m_connection.disconnect();

    m_condition.notify_all();
    m_update_thread.join();

    delete m_speech_impl;
    delete m_history;
    delete m_loader;
}


void ReviewManager::review()
{
    std::string action;
    boost::timer::cpu_timer t;
    ReviewString n;

    m_history->initialize();
    m_history->synchronize_history( m_loader->get_string_hash_set() );
    set_title();
    update();
    m_update_thread = boost::thread( boost::bind( &ReviewManager::update_thread, this ) );

    while ( m_running )
    {
        n = get_next();
        m_current_reviewing = &n;

        do
        {
            LOG_TRACE << "begin do";

            t.start();
            action = n.review();

            if ( action == "next" )
            {
                action = wait_user_interaction();
                Utility::cls();
            }

            while ( action == "back" )
            {
                Utility::cls();
                n = get_previous();
                m_current_reviewing = &n;
                action = n.review();

                if ( action == "next" )
                {
                    action = wait_user_interaction();
                    Utility::cls();
                }
            }

            if ( action == "quit" )
            {
                m_running = false;
                break;
            }

            while ( action == "speech" )
            {
                n.play_speech();
                action = wait_user_interaction();
            }

            if ( action == "listen" )
            {
                m_is_listening = true;
                boost::thread t( boost::bind( &ReviewManager::listen_thread, this ) );
                action = wait_user_interaction();
                m_is_listening = false;
            }

            if ( action == "delete" )
            {
                m_history->disable( n.get_hash() );
            }

            if ( action == "add-to-group" )
            {
                m_review_group.push_back( n );
            }

            if ( m_running && m_speech && m_play_back )
            {
                m_play_back_string.push_back( n );

                while ( m_play_back < m_play_back_string.size() )
                {
                    m_play_back_string.pop_front();
                }

                std::vector<std::string> w;

                for ( std::list<ReviewString>::iterator it = m_play_back_string.begin(); it != m_play_back_string.end(); ++it )
                {
                    std::vector<std::string> w2 = Utility::extract_strings_in_braces( it->get_string() );
                    w.insert( w.end(), w2.begin(), w2.end() );
                }

                std::vector< std::pair<std::string, std::string> > word_paths = m_speech->get_word_speech_file_path( w );
                Utility::play_or_tts_list_thread( word_paths );
            }

            m_current_reviewing = NULL;
            n.m_speech = NULL;
            LOG_TRACE << "end do";
        }
        while ( m_running && ( t.elapsed().wall < m_minimal_review_time ) );
    }
}


ReviewString ReviewManager::get_next()
{
    LOG_TRACE << "begin";

    show_next_picture();

    boost::unique_lock<boost::mutex> lock( m_mutex );

    m_review_mode = Forward;

    if ( m_reviewing_list.empty() )
    {
        update();

        if ( m_reviewing_list.empty() )
        {
            set_title();
            return ReviewString();
        }
    }

    for ( std::list<ReviewString>::iterator it = m_review_group.begin(); it != m_review_group.end(); ++it )
    {
        size_t hash = it->get_hash();

        if ( m_minimal_review_distance < m_review_number - m_hash_number_map[hash] )
        {
            ReviewString s = *it;
            m_review_group.erase( it );
            s.m_speech = m_speech;
            m_hash_number_map[hash] = m_review_number++;
            return s;
        }
    }

    size_t hash = get_next_hash( m_reviewing_list, get_next_order( m_review_orders, m_review_order_it ) );
    m_reviewing_set.erase( hash );
    m_review_history.push_back( hash );
    m_history->save_history( hash, std::time(0) );

    if ( m_reviewing_set.empty() )
    {
        m_history->write_history();
        m_history->clean_review_cache();
    }

    if ( m_auto_update_interval )
    {
        m_condition.notify_one();
    }

    m_hash_number_map[hash] = m_review_number++;

    LOG_TRACE << "end";
    set_title();
    return ReviewString( hash, m_loader, m_history, m_speech, m_display_format );
}


ReviewString ReviewManager::get_previous()
{
    boost::unique_lock<boost::mutex> lock( m_mutex );

    if ( m_review_history.empty() )
    {
        return ReviewString();
    }

    if ( m_review_mode == Forward )
    {
        m_review_mode = Backward;

        if ( m_review_history.size() == 1 )
        {
            m_backward_index = 0;
        }
        else
        {
            m_backward_index = m_review_history.size() - 1;
        }
    }

    if ( 0 < m_backward_index )
    {
        m_backward_index--;
    }

    return ReviewString( m_review_history[m_backward_index], m_loader, m_history, m_speech, m_display_format );
}


std::string ReviewManager::wait_user_interaction()
{
    static HANDLE stdinput = GetStdHandle(STD_INPUT_HANDLE);
    static INPUT_RECORD input_buffer[128];
    static DWORD num_read = 0;

    Utility::disable_console_mode( ENABLE_QUICK_EDIT_MODE  );
    SetConsoleMode( stdinput, ENABLE_WINDOW_INPUT | ENABLE_MOUSE_INPUT );

    while ( true )
    {
        ReadConsoleInput( stdinput, input_buffer, 128, &num_read );

        for ( size_t i = 0; i < num_read; i++)
        {
            switch( input_buffer[i].EventType )
            {
            case KEY_EVENT:
                {
                    KEY_EVENT_RECORD& e = input_buffer[i].Event.KeyEvent;
                    if ( e.bKeyDown )
                    {
                        switch ( e.wVirtualKeyCode )
                        {
                        case VK_DELETE:         // Del
                            return "delete";
                        case VK_ESCAPE:         // Esc
                            return "quit";
                        case VK_LEFT:           // Left
                        case VK_UP:             // Up
                        case VK_PRIOR:          // PgUp
                        case VK_BROWSER_BACK:   // Back
                        case VK_OEM_MINUS:      // -
                        case VK_OEM_4:          // [{
                        case VK_OEM_1:          //;:
                        case VK_OEM_COMMA:      // ,<
                            return "back";
                        }
                        return "next";
                    }
                    break;
                }
            case MOUSE_EVENT:
                {
                    MOUSE_EVENT_RECORD& e = input_buffer[i].Event.MouseEvent;
                    switch (e.dwEventFlags)
                    {
                    case 0:
                        switch( e.dwButtonState )
                        {
                        case RIGHTMOST_BUTTON_PRESSED:
                            return "back";
                        case FROM_LEFT_1ST_BUTTON_PRESSED:
                            return "next";
                        }
                        break;
                    case DOUBLE_CLICK:
                    case MOUSE_HWHEELED:
                    case MOUSE_WHEELED:
                        return "next";
                    case MOUSE_MOVED:
                    default:
                        break;
                    }
                    break;
                }
            }
        }
    }

    return "next";
}


void ReviewManager::set_title()
{
    std::stringstream strm;
    strm << m_file_name << " - ";
    
    if ( 0 == m_reviewing_set.size() && m_history->is_finished() )
    {
        strm << "finished.";
    }
    else
    {
        strm << m_reviewing_set.size();
    }

    SetConsoleTitle( strm.str().c_str() );
}


void ReviewManager::update()
{
    LOG_TRACE << "begin";

    const std::set<size_t>& all = m_loader->get_string_hash_set();

    if ( m_all.size() != all.size() ) // no need precise
    {
        m_all = all;
        m_history->synchronize_history( m_all );
    }

    std::set<size_t> expired = m_history->get_expired();

    if ( m_reviewing_set.size() != expired.size() )
    {
        if ( ! m_reviewing_set.empty() )
        {
            LOG_DEBUG
                << "old-size=" << m_reviewing_list.size()
                << ", new-size=" << expired.size() << ""
                << get_new_expired_string( m_reviewing_set, expired )
                ;
        }
        else
        {
            LOG_DEBUG
                << "old-size=" << m_reviewing_list.size()
                << ", new-size=" << expired.size() << ""
                ;
        }

        m_reviewing_set = expired;
        m_reviewing_list.assign( m_reviewing_set.begin(), m_reviewing_set.end() );

        {
            static Order order( m_loader, m_history );
            m_reviewing_list.sort( order );
        }

        set_title();
        LOG_DEBUG << "sort " << m_reviewing_list.size();
        // LOG_TEST<< std::endl << get_hash_list_string( m_reviewing_list );
    }

    LOG_TRACE << "end";
}


void ReviewManager::update_thread()
{
    while ( m_running )
    {
        if ( 0 == m_auto_update_interval )
        {
            boost::unique_lock<boost::mutex> lock( m_mutex );
            while ( 0 == m_auto_update_interval )
            {
                m_condition.wait( lock );
            }
        }

        if ( !m_running )
        {
            break;
        }

        boost::unique_lock<boost::mutex> lock( m_mutex );
        boost::chrono::seconds interval( m_auto_update_interval );
        m_condition.wait_for( lock, interval );

        if ( !m_running )
        {
            break;
        }

        if ( ! m_is_listening )
        {
            LOG_TRACE << "begin";
            LOG_DEBUG << "update thread";
            update();
            LOG_TRACE << "end";
        }
    }
}


std::ostream& ReviewManager::output_hash_list( std::ostream& os, const std::list<size_t>& l )
{
    for ( std::list<size_t>::const_iterator it = l.begin(); it != l.end(); ++it )
    {
        size_t hash = *it;
        std::time_t t = m_history->get_last_review_time( hash );
        size_t r = m_history->get_review_round( hash );
        const std::string& s = m_loader->get_string( hash );

        os
            << r << "\t"
            << Utility::string_from_time_t( t ) << "\t"
            << s.size() << "\t"
            << s << "\n";
            ;
    }

    return os;
}


std::string ReviewManager::get_hash_list_string( const std::list<size_t>& l )
{
    std::stringstream strm;
    output_hash_list( strm, l );
    return strm.str();
}


std::string ReviewManager::get_new_expired_string( const std::set<size_t>& os,  const std::set<size_t>& ns )
{
    std::set<size_t> added;
    std::set_difference( ns.begin(), ns.end(), os.begin(), os.end(), std::inserter( added, added.begin() ) );

    std::stringstream strm;

    for ( std::set<size_t>::iterator it = added.begin(); it != added.end(); ++it )
    {
        size_t hash = *it;
        size_t round = m_history->get_review_round( hash );
        std::time_t last_review = m_history->get_last_review_time( hash );
        std::time_t elapsed = std::time(0) - last_review;
        const std::string& s = m_loader->get_string( *it );
        strm << std::endl << "expired: " << round << " (" << Utility::duration_string_from_seconds(elapsed) << ") " << s;
    }

    return strm.str();
}


void ReviewManager::listen_thread()
{
    if ( m_listening_list.empty() )
    {
        boost::unique_lock<boost::mutex> lock( m_mutex );

        update();

        if ( m_listen_all )
        {
            m_listening_list.assign( m_all.begin(), m_all.end() );
            m_listening_list.erase( std::remove_if( m_listening_list.begin(),
                                                    m_listening_list.end(),
                                                    boost::bind( &History::is_not_reviewable, m_history, _1 ) ),
                                    m_listening_list.end() );
        }
        else
        {
            m_listening_list = m_reviewing_list;
        }
    }

    std::vector<EReviewOrder> listen_orders = m_review_orders;
    std::vector<EReviewOrder>::iterator listen_orders_it = listen_orders.begin();

    while ( m_is_listening && ! m_listening_list.empty() )
    {
        size_t hash = get_next_hash( m_listening_list, get_next_order( listen_orders, listen_orders_it ) );
        const std::string& s = m_loader->get_string( hash );
        std::vector<std::string> words = Utility::extract_strings_in_braces( s );

        if ( words.empty() )
        {
            continue;
        }

        if ( m_speech == NULL )
        {
            break;
        }

        std::vector< std::pair<std::string, std::string> > word_paths = m_speech->get_word_speech_file_path( words );

        if ( word_paths.empty() )
        {
            continue;
        }

        Utility::cls();
        std::stringstream strm;
        strm << "TITLE listen - " << m_listening_list.size();
        std::string title = strm.str();
        SetConsoleTitleA( strm.str().c_str() );
        //system( ( "TITLE listen - " + boost::lexical_cast<std::string>( m_listening_list.size() ) ).c_str() );

        if ( ! m_listen_no_string )
        {
            std::string ts = s;
            ts.erase( std::remove_if( ts.begin(), ts.end(), boost::is_any_of( "{}" ) ), ts.end() );
            std::cout << "\t" << ts << std::endl;
        }

        std::cout << "\t";
        std::copy( words.begin(), words.end(), std::ostream_iterator<std::string>( std::cout, "\n\t" ) );
        LOG_TRACE << s;
        Utility::play_or_tts_list( word_paths );
    }

    set_title();
}


void ReviewManager::update_option( const boost::program_options::variables_map& vm )
{
    static OptionUpdateHelper option_helper;

    if ( option_helper.update_one_option<boost::timer::nanosecond_type>( review_minimal_time_option, vm, 500 ) )
    {
        m_minimal_review_time = option_helper.get_value<boost::timer::nanosecond_type>( review_minimal_time_option ) * 1000 * 1000;
        LOG_DEBUG << "review-minimal-review-time: " << m_minimal_review_time;
    }

    if ( option_helper.update_one_option<size_t>( review_auto_update_interval_option, vm, 60 ) )
    {
        volatile size_t old_value = m_auto_update_interval;
        m_auto_update_interval = option_helper.get_value<size_t>( review_auto_update_interval_option );
        LOG_DEBUG << "review-auto-update-interval: " << m_auto_update_interval;

        if ( 0 == old_value )
        {
            m_condition.notify_one();
        }
    }

    if ( option_helper.update_one_option<size_t>( speech_play_back, vm, 0 ) )
    {
        m_play_back = option_helper.get_value<size_t>( speech_play_back );
        LOG_DEBUG << "speech-play-back: " << m_play_back;

        if ( 0 == m_play_back && ! m_play_back_string.empty() )
        {
            m_play_back_string.clear();
        }
    }

    if ( option_helper.update_one_option<std::string>( review_order_option, vm, "latest" ) )
    {
        boost::unique_lock<boost::mutex> lock( m_mutex );
        std::string order_option_str = option_helper.get_value<std::string>( review_order_option );
        m_review_orders = convert_from_string( order_option_str );
        m_review_order_it = m_review_orders.begin();
        LOG_DEBUG << "review-order: " << order_option_str;
    }

    if ( option_helper.update_one_option<std::string>( speech_disabled_option, vm, "false" ) )
    {
        bool speech_disabled = ( "true" == option_helper.get_value<std::string>( speech_disabled_option ) );

        if ( speech_disabled )
        {
            m_speech = NULL;
            if ( m_current_reviewing != NULL ) { m_current_reviewing->m_speech = NULL; }
            LOG_DEBUG << "speech-disabled: true";
        }
        else
        {
            m_speech = m_speech_impl;
            if ( m_current_reviewing != NULL ) { m_current_reviewing->m_speech = m_speech; }
            LOG_DEBUG << "speech-disabled: false";
        }
    }

    if ( option_helper.update_one_option<std::string>( listen_no_string_option, vm, "false" ) )
    {
        m_listen_no_string = ( "true" == option_helper.get_value<std::string>( listen_no_string_option ) );
        LOG_DEBUG << "listen-no-string: " << ( m_listen_no_string ? "true" : "false" );
    }

    if ( option_helper.update_one_option<std::string>( listen_all_option, vm, "false" ) )
    {
        m_listen_all = ( "true" == option_helper.get_value<std::string>( listen_all_option ) );
        LOG_DEBUG << "listen-all: " << ( m_listen_all ? "true" : "false" );
    }

    if ( option_helper.update_one_option<std::string>( file_name_option, vm ) )
    {
        m_file_name = option_helper.get_value<std::string>( file_name_option );
        LOG_DEBUG << "file-name: " << m_file_name;
    }

    if ( option_helper.update_one_option<std::string>( review_display_format_option, vm, "Q,AE" ) )
    {
        m_display_format = option_helper.get_value<std::string>( review_display_format_option );
        LOG_DEBUG << "review-display-format: " << m_display_format;
    }

    if ( option_helper.update_one_option<size_t>( review_minimal_review_distance_option, vm, 10 ) )
    {
        m_minimal_review_distance = option_helper.get_value<size_t>( review_minimal_review_distance_option );
        LOG_DEBUG << "review-minimal-review-distance: " << m_minimal_review_distance;
    }

    if ( option_helper.update_one_option<std::string>( system_picture_path, vm ) )
    {
        m_picture_path = option_helper.get_value<std::string>( system_picture_path );
        LOG_DEBUG << "system-picture-path: " << m_picture_path;

        if ( boost::filesystem::exists( m_picture_path ) )
        {
            m_picture_dir_it = boost::filesystem::recursive_directory_iterator( m_picture_path );
        }
        else
        {
            m_picture_dir_it = boost::filesystem::recursive_directory_iterator();
        }
    }
}


size_t ReviewManager::get_next_hash( std::list<size_t>& hash_list, EReviewOrder order )
{
    if ( hash_list.empty() )
    {
        return 0;
    }

    size_t hash = 0;

    if ( order == Latest )
    {
        hash = hash_list.front();
        hash_list.pop_front();
    }
    else if ( order == Earliest )
    {
        hash = hash_list.back();
        hash_list.pop_back();
    }
    else if ( order == Random )
    {
        if ( hash_list.size() < 3 )
        {
            hash = hash_list.back();
            hash_list.pop_back();
        }
        else
        {
            std::list<size_t>::iterator it = hash_list.begin();
            std::advance( it, Utility::random_number( 1, hash_list.size() - 2 ) );
            hash = *it;
            hash_list.erase( it );
        }
    }
    else if ( order == Middle )
    {
        std::list<size_t>::iterator it = hash_list.begin();
        std::advance( it, hash_list.size() / 2 );
        hash = *it;
        hash_list.erase( it );
    }
    else
    {
        LOG << "error: the review order is unknown: " << order;
    }

    return hash;
}


ReviewManager::EReviewOrder ReviewManager::get_next_order( std::vector<EReviewOrder>& orders, std::vector<EReviewOrder>::iterator& it )
{
    if ( it == orders.end() )
    {
        it = orders.begin();
    }

    if ( it == orders.end() )
    {
        return Invalid;
    }

    EReviewOrder order = *it;
    it++;

    static const char* order_str[] = { "Latest", "Earliest", "Random", "Middle", "Invalid" };
    LOG_TRACE << "review order: " << order_str[order];
    return order;
}


std::vector<ReviewManager::EReviewOrder> ReviewManager::convert_from_string( const std::string& order_string )
{
    std::vector<ReviewManager::EReviewOrder> orders;
    std::vector<std::string> vos;
    boost::split( vos, order_string, boost::is_any_of( ",:;-/\\%@|" ) );

    for ( size_t i = 0; i < vos.size(); ++i )
    {
        boost::to_lower(vos[i]);
        if ( vos[i] == "latest" )       { orders.push_back( Latest ); }
        else if ( vos[i] == "earlist" ) { orders.push_back( Earliest ); }
        else if ( vos[i] == "random" )  { orders.push_back( Random ); }
        else if ( vos[i] == "middle" )  { orders.push_back( Middle ); }
        else
        {
            LOG << "invalid order: " << vos[i];
        }
    }

    return orders;
}


void ReviewManager::upgrade_hash_algorithm()
{
    std::cout << "upgrading hash algorithm for " << m_loader->m_file_name << " ..." << std::flush;

    m_history->initialize();

    history_type& old_history = m_history->m_history;
    boost::function<size_t (const std::string&)> new_hash_fun = &Loader::string_hash_new;
    const std::set<size_t>& hash_set = m_loader->get_string_hash_set();
    history_type new_history;

    for ( std::set<size_t>::const_iterator it = hash_set.begin(); it != hash_set.end(); ++it )
    {
        size_t hash = *it;
        const std::string& s = m_loader->get_string_no_lock( hash );

        history_type::iterator find_it = old_history.find( hash );

        if ( find_it == old_history.end() )
        {
            std::cout << "\naborted: not match (updated already?)" << std::endl;
            return;
        }

        size_t new_hash = new_hash_fun( s );
        new_history[new_hash] = find_it->second;
    }

    if ( old_history == new_history )
    {
        std::cout << "\nthe algorithm is same, do nothing." << std::endl;
        return;
    }

    m_history->m_history = new_history;
    m_history->write_history();
    std::cout << "\ndone." << std::endl;
}


void ReviewManager::show_next_picture( const std::string& path )
{
    boost::filesystem::recursive_directory_iterator& it = m_picture_dir_it;
    boost::filesystem::recursive_directory_iterator end;

    if ( it != end )
    {
        if ( ! is_directory( it.status() )  )
        {
            if ( path.empty() )
            {
                Utility::set_system_wallpaper( it->path().string() );
            }
            else
            {
                Utility::set_system_wallpaper( path );
                return;
            }
        }

        size_t step = Utility::random_number( 1, 100 );

        for ( size_t i = 0; i < step; ++i )
        {
            if ( ++it == end )
            {
                it = boost::filesystem::recursive_directory_iterator( m_picture_path );
            }
        }
    }
}
