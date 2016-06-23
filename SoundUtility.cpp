#include "stdafx.h"
#include "SoundUtility.h"
#include "QueueProcessor.h"


namespace Utility
{

    void play_sound( const std::string& file  )
    {
        try
        {
            IGraphBuilder* graph = NULL;
            IMediaControl* control = NULL;
            IMediaEvent*   evnt = NULL;

            ::CoCreateInstance( CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER, IID_IGraphBuilder, (void **)&graph );
            graph->QueryInterface( IID_IMediaControl, (void **)&control );
            graph->QueryInterface( IID_IMediaEvent, (void **)&evnt );

            HRESULT hr = graph->RenderFile( boost::locale::conv::utf_to_utf<wchar_t>(file).c_str(), NULL );

            if ( SUCCEEDED(hr) )
            {
                hr = control->Run();

                if ( SUCCEEDED(hr) )
                {
                    long code;
                    evnt->WaitForCompletion( INFINITE, &code );
                }
            }

            evnt->Release();
            control->Release();
            graph->Release();
        }
        catch ( ... )
        {
            //LOG << "error " << file;
        }
    }


    void play_sound_list( const std::vector<std::string>& files )
    {
        for ( size_t i = 0; i < files.size(); ++i )
        {
            play_sound( files[i] );
        }
    }


    void play_sound_list_thread( const std::vector<std::string>& files )
    {
        static QueueProcessor<> player( ( boost::function<void (const std::vector<std::string>&)>( &play_sound_list ) ) );
        player.queue_items( files );
    }


    void text_to_speech( const std::string& word )
    {
        std::vector<std::string> words;
        words.push_back( word );
        text_to_speech_list( words );
    }


    void text_to_speech_list( const std::vector<std::string>& words )
    {
        static ISpVoice* sp_voice = NULL;

        if ( NULL == sp_voice )
        {
            ::CoCreateInstance( CLSID_SpVoice, NULL, CLSCTX_ALL, IID_ISpVoice, (void **)&sp_voice );
        }

        for ( size_t i = 0; i < words.size(); ++i )
        {
            // std::wstring ws = boost::locale::conv::utf_to_utf<wchar_t>( words[i], "" );
            std::wstring ws = boost::locale::conv::to_utf<wchar_t>( words[i], "GBK" );
            sp_voice->Speak( ws.c_str(), 0, NULL );
            //LOG_TRACE << words[i];

            if ( i + 1 < words.size() )
            {
                boost::this_thread::sleep_for( boost::chrono::milliseconds(300) );
            }
        }
    }


    void text_to_speech_list_thread( const std::vector<std::string>& words )
    {
        static QueueProcessor<> speaker( ( boost::function<void (const std::vector<std::string>&)>( &text_to_speech_list ) ) );
        speaker.queue_items( words );
    }


    void play_or_tts( const std::pair<std::string, std::string>& word_path )
    {
        if ( ! word_path.second.empty() )
        {
            play_sound( word_path.second );
        }
        else
        {
            text_to_speech( word_path.first );
        }
    }


    void play_or_tts_list( const std::vector< std::pair<std::string, std::string> >& word_path_list )
    {
        for ( size_t i = 0; i < word_path_list.size(); ++i )
        {
            play_or_tts( word_path_list[i] );
        }
    }

    void play_or_tts_list_thread( const std::vector< std::pair<std::string, std::string> >& word_path_list )
    {
        static QueueProcessor< std::pair<std::string, std::string> > play_tts( ( boost::function<void (const std::vector< std::pair<std::string, std::string> >&)>( &play_or_tts_list ) ) );
        play_tts.queue_items( word_path_list );
    }


    RecordSound::RecordSound( const std::string& n )
        : m_file_name( n )
    {
        ::mciSendString( "set wave samplespersec 11025", "", 0, 0 );
        ::mciSendString( "set wave channels 2", "", 0, 0 );
        ::mciSendString( "close my_wav_sound", 0, 0, 0 );
        ::mciSendString( "open new type WAVEAudio alias my_wav_sound", 0, 0, 0 );
        ::mciSendString( "record my_wav_sound", 0, 0, 0 );
        //LOG_DEBUG << "recording bein" << m_file_name;
    }


    RecordSound::~RecordSound()
    {
        std::string s = "save my_wav_sound " + m_file_name;
        ::mciSendString( "stop my_wav_sound", 0, 0, 0 );
        ::mciSendString( s.c_str(), 0, 0, 0 );
        ::mciSendString( "close my_wav_sound", 0, 0, 0 );
        //LOG_DEBUG << "recording end" << m_file_name;
    }

}
