#include "stdafx.h"
#include "SoundUtility.h"
#include "QueueProcessor.h"


namespace Utility
{

    void play_sound( const std::wstring& file  )
    {
        try
        {
            IGraphBuilder* graph = NULL;
            IMediaControl* control = NULL;
            IMediaEvent*   evnt = NULL;

            ::CoCreateInstance( CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER, IID_IGraphBuilder, (void **)&graph );
            graph->QueryInterface( IID_IMediaControl, (void **)&control );
            graph->QueryInterface( IID_IMediaEvent, (void **)&evnt );

            HRESULT hr = graph->RenderFile( file.c_str(), NULL );

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


    void play_sound_list( const std::vector<std::wstring>& files )
    {
        for ( size_t i = 0; i < files.size(); ++i )
        {
            play_sound( files[i] );
        }
    }


    void play_sound_list_thread( const std::vector<std::wstring>& files )
    {
        static QueueProcessor<> player( ( boost::function<void (const std::vector<std::wstring>&)>( &play_sound_list ) ) );
        player.queue_items( files );
    }


    void text_to_speech( const std::wstring& word )
    {
        std::vector<std::wstring> words;
        words.push_back( word );
        text_to_speech_list( words );
    }


    void text_to_speech_list( const std::vector<std::wstring>& words )
    {
        static ISpVoice* sp_voice = NULL;

        if ( NULL == sp_voice )
        {
            ::CoCreateInstance( CLSID_SpVoice, NULL, CLSCTX_ALL, IID_ISpVoice, (void **)&sp_voice );
        }

        for ( size_t i = 0; i < words.size(); ++i )
        {
            // std::wstring ws = boost::locale::conv::utf_to_utf<wchar_t>( words[i], "" );
            sp_voice->Speak( words[i].c_str(), 0, NULL );
            //LOG_TRACE << words[i];

            if ( i + 1 < words.size() )
            {
                boost::this_thread::sleep_for( boost::chrono::milliseconds(300) );
            }
        }
    }


    void text_to_speech_list_thread( const std::vector<std::wstring>& words )
    {
        static QueueProcessor<> speaker( ( boost::function<void (const std::vector<std::wstring>&)>( &text_to_speech_list ) ) );
        speaker.queue_items( words );
    }


    void play_or_tts( const std::pair<std::wstring, std::wstring>& word_path )
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


    void play_or_tts_list( const std::vector< std::pair<std::wstring, std::wstring> >& word_path_list )
    {
        for ( size_t i = 0; i < word_path_list.size(); ++i )
        {
            play_or_tts( word_path_list[i] );
        }
    }

    void play_or_tts_list_thread( const std::vector< std::pair<std::wstring, std::wstring> >& word_path_list )
    {
        static QueueProcessor< std::pair<std::wstring, std::wstring> > play_tts( ( boost::function<void (const std::vector< std::pair<std::wstring, std::wstring> >&)>( &play_or_tts_list ) ) );
        play_tts.queue_items( word_path_list );
    }


    RecordSound::RecordSound( const std::wstring& n )
        : m_file_name( n )
    {
        ::mciSendString( L"set wave samplespersec 11025", L"", 0, 0 );
        ::mciSendString( L"set wave channels 2", L"", 0, 0 );
        ::mciSendString( L"close my_wav_sound", 0, 0, 0 );
        ::mciSendString( L"open new type WAVEAudio alias my_wav_sound", 0, 0, 0 );
        ::mciSendString( L"record my_wav_sound", 0, 0, 0 );
        //LOG_DEBUG << "recording bein" << m_file_name;
    }


    RecordSound::~RecordSound()
    {
        std::wstring s = L"save my_wav_sound " + m_file_name;
        ::mciSendString( L"stop my_wav_sound", 0, 0, 0 );
        ::mciSendString( s.c_str(), 0, 0, 0 );
        ::mciSendString( L"close my_wav_sound", 0, 0, 0 );
        //LOG_DEBUG << "recording end" << m_file_name;
    }

}
