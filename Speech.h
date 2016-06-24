#pragma once


class Speech
{
public:

    Speech();
    void play( const std::vector<std::wstring>& words );
    std::vector<std::wstring> get_files( const std::vector<std::wstring>& words, std::vector<std::wstring>& speak_words );
    std::vector< std::pair<std::wstring, std::wstring> > get_word_speech_file_path( const std::vector<std::wstring>& words );
    void update_option( const boost::program_options::variables_map& vm ); // ProgramOptions slot

public:

    boost::mutex m_mutex;
    volatile bool m_no_duplicate;
    volatile bool m_no_text_to_speech;
    volatile size_t m_text_to_speech_repeat;
    std::vector< std::pair<boost::filesystem::path, std::wstring> > m_paths;
};
