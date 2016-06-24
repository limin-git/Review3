#include "stdafx.h"
#include "Speech.h"
#include "Utility.h"
#include "Log.h"
#include "OptionString.h"
#include "ProgramOptions.h"
#include "OptionUpdateHelper.h"


Speech::Speech()
    : m_no_duplicate( false ),
      m_no_text_to_speech( false ),
      m_text_to_speech_repeat( 1 )
{
    ProgramOptions::connect_to_signal( boost::bind( &Speech::update_option, this, _1 ) );
}


void Speech::play( const std::vector<std::wstring>& words )
{
    std::vector< std::pair<std::wstring, std::wstring> > word_paths = get_word_speech_file_path( words );

    if ( ! word_paths.empty() )
    {
        Utility::play_or_tts_list_thread( word_paths );
    }
}


std::vector<std::wstring> Speech::get_files( const std::vector<std::wstring>& words, std::vector<std::wstring>& speak_words )
{
    boost::unique_lock<boost::mutex> lock( m_mutex );

    std::vector<std::wstring> files;
    std::vector<bool> word_found( words.size() );
    for ( size_t i = 0; i < word_found.size(); ++i ) { word_found[i] = false; }

    for ( size_t i = 0; i < m_paths.size(); ++i )
    {
        for ( size_t j = 0; j < words.size(); ++j )
        {
            if ( m_no_duplicate && word_found[j] == true )
            {
                continue;
            }

            std::wstring first_char = words[j].substr( 0, 1 );
            boost::filesystem::path path = m_paths[i].first / first_char / ( words[j] + m_paths[i].second );

            if ( boost::filesystem::exists( path ) )
            {
                files.push_back( path.wstring() );
                word_found[j] = true;
                LOG_TRACE << path.wstring();
            }
        }
    }

    for ( size_t i = 0; i < word_found.size(); ++i )
    {
        if ( ! word_found[i] )
        {
            speak_words.push_back( words[i] );
        }
    }

    return files;
}


std::vector< std::pair<std::wstring, std::wstring> > Speech::get_word_speech_file_path( const std::vector<std::wstring>& words )
{
    boost::unique_lock<boost::mutex> lock( m_mutex );

    std::vector< std::pair<std::wstring, std::wstring> > paths;

    if ( m_no_duplicate )
    {
        for ( size_t i = 0; i < words.size(); ++i )
        {
            const std::wstring& word = words[i];
            std::wstring word_path;
            std::wstring first_char = words[i].substr( 0, 1 );

            for ( size_t j = 0; j < m_paths.size(); ++j )
            {
                boost::filesystem::path path = m_paths[j].first / first_char / ( word + m_paths[j].second );

                if ( boost::filesystem::exists( path ) )
                {
                    word_path = path.wstring();
                    LOG_TRACE << word_path;
                    break;
                }
                else
                {
                    LOG_TRACE << "can not find: " << path.wstring();
                }
            }

            if ( ! word_path.empty() || ! m_no_text_to_speech )
            {
                paths.push_back( std::make_pair(word, word_path ) );
            }
        }
    }
    else
    {
        for ( size_t i = 0; i < m_paths.size(); ++i )
        {
            for ( size_t j = 0; j < words.size(); ++j )
            {
                const std::wstring& word = words[j];
                std::wstring first_char = word.substr( 0, 1 );
                std::wstring word_path;
                boost::filesystem::path path = m_paths[i].first / first_char / ( word + m_paths[i].second );

                if ( boost::filesystem::exists( path ) )
                {
                    word_path = path.wstring();
                    LOG_TRACE << word_path;
                }
                else
                {
                    LOG_TRACE << "can not find: " << path.wstring();
                }

                if ( ! word_path.empty() || ! m_no_text_to_speech )
                {
                    paths.push_back( std::make_pair(word, word_path ) );
                }
            }
        }
    }

    return paths;
}


void Speech::update_option( const boost::program_options::variables_map& vm )
{
    static OptionUpdateHelper option_helper;

    if ( option_helper.update_one_option< std::vector<std::wstring> >( speech_path_option, vm ) )
    {
        std::vector<std::wstring> vs = option_helper.get_value< std::vector<std::wstring> >( speech_path_option );
        std::vector< std::pair<boost::filesystem::path, std::wstring> > paths;

        for ( size_t i = 0; i < vs.size(); ++i )
        {
            std::wstring path = vs[i];
            std::wstring extension = L".mp3";
            size_t pos = vs[i].find_first_of( L"|?*" );
            wchar_t seperator = L' ';

            if ( pos != std::wstring::npos )
            {
                path = boost::trim_copy( vs[i].substr( 0, pos ) );
                extension = boost::trim_copy( vs[i].substr( pos + 1, std::wstring::npos ) );
                seperator = vs[i][pos];
            }

            if ( boost::filesystem::exists( path ) )
            {
                paths.push_back( std::make_pair( path, extension ) );
                LOG_DEBUG << "speech path: " << path << " " << seperator << " " << extension;
            }
            else
            {
                LOG_DEBUG << "invalide path: " << vs[i];
            }
        }

        boost::unique_lock<boost::mutex> lock( m_mutex );
        m_paths = paths;
    }

    if ( option_helper.update_one_option<std::wstring>( speech_no_duplicate, vm, L"false" ) )
    {
        m_no_duplicate = ( L"true" == option_helper.get_value<std::wstring>( speech_no_duplicate ) );
        LOG_DEBUG << "speech-no-duplicate: " << m_no_duplicate;
    }

    if ( option_helper.update_one_option<std::wstring>( speech_no_text_to_speech, vm, L"false" ) )
    {
        m_no_text_to_speech = ( L"true" == option_helper.get_value<std::wstring>( speech_no_text_to_speech ) );
        LOG_DEBUG << "speech-no-text-to-speech: " << m_no_text_to_speech;
    }
}
