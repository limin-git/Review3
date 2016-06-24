#pragma once
class History;
class Loader;
class Speech;


class ReviewString
{
public:

    ReviewString( size_t hash = 0, Loader* loader = NULL, History* history = NULL, Speech* play = NULL, const std::wstring& display_format = L"" );
    std::wstring review();
    void play_speech();
    const std::wstring& get_string();
    size_t get_hash() { return m_hash; }

public:

    size_t m_hash;
    Loader* m_loader;
    History* m_history;
    Speech* m_speech;
    std::wstring m_string;
    std::wstring m_display_format;
    std::vector<std::wstring> m_speech_words;
    std::map<wchar_t, std::wstring> m_string_map;
};

typedef boost::shared_ptr<ReviewString> ReviewStringPtr;
