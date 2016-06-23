#pragma once


class Speech
{
public:

    Speech();
    void play( const std::vector<std::string>& words );
    std::vector<std::string> get_files( const std::vector<std::string>& words, std::vector<std::string>& speak_words );
    std::vector< std::pair<std::string, std::string> > get_word_speech_file_path( const std::vector<std::string>& words );
    void update_option( const boost::program_options::variables_map& vm ); // ProgramOptions slot

public:

    boost::mutex m_mutex;
    volatile bool m_no_duplicate;
    volatile bool m_no_text_to_speech;
    volatile size_t m_text_to_speech_repeat;
    std::vector< std::pair<boost::filesystem::path, std::string> > m_paths;
};
