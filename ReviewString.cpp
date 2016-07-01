#include "stdafx.h"
#include "ReviewString.h"
#include "Loader.h"
#include "History.h"
#include "Speech.h"
#include "Utility.h"
#include "Log.h"
#include "ReviewManager.h"
#include "WriteConsoleHelper.h"


ReviewString::ReviewString( size_t hash, Loader* loader, History* history, Speech* play, const std::wstring& display_format )
    : m_hash( hash ),
      m_loader( loader ),
      m_history( history ),
      m_speech( play ),
      m_display_format( display_format )
{
    if ( m_hash && m_loader && m_speech )
    {
        m_speech_words = Utility::extract_strings_in_braces( m_loader->get_string( m_hash ) );
    }

    if ( m_hash && m_loader )
    {
        m_wstring = m_loader->get_string( m_hash );
        m_wstring.erase( std::remove_if( m_wstring.begin(), m_wstring.end(), boost::is_any_of( "{}" ) ), m_wstring.end() );

        static boost::wregex e ( L"(?x) ( \\[ [a-zA-Z] \\] ) (.*?) (?= \\[ [a-zA-Z] \\] | \\z )" );
        boost::wsregex_iterator it( m_wstring.begin(), m_wstring.end(), e );
        boost::wsregex_iterator end;

        for ( ; it != end; ++it )
        {
            wchar_t ch = it->str(1)[1];
            std::wstring content = boost::trim_copy( it->str(2) );
            m_string_map[ch] = content;
        }
    }
}


std::wstring ReviewString::review()
{
    if ( 0 == m_hash )
    {
        stdcout << "empty." << "\n";
        return L"next";
    }

    if ( m_speech )
    {
        play_speech();
    }

    if ( m_string_map.empty() || m_display_format.empty() )
    {
        Utility::write_console_on_center( m_wstring );
    }
    else
    {
        bool should_wait = false;
        bool should_new_line = false;
        bool is_first_part = true;
        std::wstring first_content;

        for ( size_t i = 0; i < m_display_format.size(); ++i )
        {
            wchar_t ch = m_display_format[i];

            if ( ( ch == ',' ) && should_wait )
            {
                std::wstring action = ReviewManager::wait_user_interaction();

                if ( action != L"next" )
                {
                    Utility::cls();
                    return action;
                }

                stdcout << L"\n";
                should_wait = false;
                should_new_line = false;
            }
            else
            {
                const std::wstring& content = m_string_map[ch];

                if ( content.empty() )
                {
                    continue;
                }

                if ( ! first_content.empty() )
                {
                    Utility::cls();
                    stdcout << L"\t" << first_content << "\n";
                    first_content.clear();
                }

                if ( should_new_line )
                {
                    stdcout << L"\n";
                }

                stdcout << L"\t";

                if ( is_first_part )
                {
                    is_first_part = false;
                    first_content = content;
                    Utility::write_console_on_center( content );
                }
                else
                {
                    stdcout << content;
                }

                should_wait = true;
                should_new_line = true;
            }
        }
    }

    if ( m_history )
    {
        LOG_DEBUG
            << m_wstring << std::endl << "\t\t"
            << "[Round: " << m_history->get_review_round( m_hash ) << "] "
            << Utility::duration_string_from_time_list( m_history->get_times( m_hash ) );
    }

    return L"next";
}


void ReviewString::play_speech()
{
    if ( m_speech )
    {
        if ( ! m_speech_words.empty() )
        {
            m_speech->play( m_speech_words );
        }
    }
}


const std::wstring& ReviewString::get_string()
{
    if ( m_loader )
    {
        return m_loader->get_string( m_hash );
    }

    static std::wstring empty;
    return empty;
}
