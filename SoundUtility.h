#pragma once


namespace Utility
{
    struct RecordSound
    {
        RecordSound( const std::string& n );
        ~RecordSound();
        std::string m_file_name;
    };

    void play_sound( const std::string& file );
    void play_sound_list( const std::vector<std::string>& files );
    void play_sound_list_thread( const std::vector<std::string>& files );
    void text_to_speech( const std::string& word );
    void text_to_speech_list( const std::vector<std::string>& words );
    void text_to_speech_list_thread( const std::vector<std::string>& words );
    void play_or_tts( const std::pair<std::string, std::string>& word_path );
    void play_or_tts_list( const std::vector< std::pair<std::string, std::string> >& word_path_list );
    void play_or_tts_list_thread( const std::vector< std::pair<std::string, std::string> >& word_path_list );

}
