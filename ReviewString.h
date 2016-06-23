#pragma once
class History;
class Loader;
class Speech;


class ReviewString
{
public:

    ReviewString( size_t hash = 0, Loader* loader = NULL, History* history = NULL, Speech* play = NULL, const std::string& display_format = "" );
    std::string review();
    void play_speech();
    const std::string& get_string();
    size_t get_hash() { return m_hash; }

public:

    size_t m_hash;
    Loader* m_loader;
    History* m_history;
    Speech* m_speech;
    std::string m_string;
    std::string m_display_format;
    std::vector<std::string> m_speech_words;
    std::map<char, std::string> m_string_map;
};

typedef boost::shared_ptr<ReviewString> ReviewStringPtr;
