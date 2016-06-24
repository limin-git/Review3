#pragma once
typedef std::set<size_t> HashSet;
typedef std::map<size_t, std::wstring> HashStringMap;


class Loader
{
public:

    Loader( boost::function<size_t (const std::wstring&)> hash_function = &Loader::string_hash );
    const std::set<size_t>& get_string_hash_set();
    const std::wstring& get_string( size_t hash );
    const std::wstring& get_string_no_lock( size_t hash ) { return m_hash_2_string_map[hash]; } // should lock ouside

public:

    void reload();
    void process_file_change(); // DirectoryWatcher slot

public:

    static size_t string_hash( const std::wstring& str );
    static size_t string_hash_new( const std::wstring& str ) { std::cout << "are you kidding?" << std::endl; ::exit(0); }

public:

    static std::wstring get_difference( const HashSet& os, const HashStringMap& om, const HashSet& ns, const HashStringMap& nm );

public:

    boost::mutex m_mutex;
    std::wstring m_file_name;
    std::time_t m_last_write_time;
    std::set<size_t> m_string_hash_set;
    std::map<size_t, std::wstring> m_hash_2_string_map;
    boost::function<size_t (const std::wstring&)> m_hash_function;
};
