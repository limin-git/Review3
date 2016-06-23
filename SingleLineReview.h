#pragma once
typedef std::vector<std::time_t> time_list;
typedef std::map<size_t, time_list> history_type;
typedef std::pair<std::string, size_t> string_hash_pair;
typedef std::vector<string_hash_pair> string_hash_list;
typedef boost::function<size_t(const std::string&)> hash_functor;


class SingleLineReview
{
    typedef SingleLineReview this_type;
    enum { minute = 60, hour = 60 * minute ,day = 24 * hour, month = 30 * day };

public:

    SingleLineReview( const std::string& file_name, const boost::program_options::variables_map& vm = boost::program_options::variables_map() );

public:

    void review();

public:

    void initialize_history();
    void reload_strings( hash_functor hasher = &this_type::string_hash );
    void synchronize_history();
    void collect_reviewing_strings();
    void collect_reviewing_strings_thread();
    std::string display_reviewing_string( const string_hash_list& strings, size_t index );
    std::string wait_for_input( const std::string& message = "" );
    void write_review_time( const string_hash_pair& s );
    void write_review_to_history();
    void on_review_begin();
    void on_review_end();
    bool write_history();
    void merge_history( const history_type& review_history );
    void update_hash_algorighom( hash_functor old_hasher, hash_functor new_hasher );
    bool view_order( const string_hash_pair& lhs, const string_hash_pair& rhs ) const;

public:

    static history_type load_history_from_file( const std::string& file_name );
    static size_t string_hash( const std::string& str );
    static int random_gen( int i ) { return std::rand() % i; }
    static std::string time_string( std::time_t t, const char* format = "%Y/%m/%d %H:%M:%S" );
    static std::string time_duration_string( std::time_t t );
    static std::string get_time_list_string( const time_list& times );
    static std::string get_history_string( const history_type& history );
    static std::ostream& output_time_list( std::ostream& os, const time_list& times );
    static std::ostream& output_history( std::ostream& os, const history_type& history );

public:

    boost::mutex m_mutex;
    history_type m_history;
    std::string m_file_name;
    std::string m_review_name;
    std::string m_history_name;
    std::ofstream m_review_strm;
    size_t m_minimal_review_time;
    size_t m_collect_interval;
    volatile bool m_is_reviewing;
    time_list m_review_spans;
    string_hash_list m_strings;
    string_hash_list m_reviewing_strings;
    boost::log::sources::logger m_log;
    boost::log::sources::logger m_log_debug;
    boost::log::sources::logger m_log_trace;
    boost::log::sources::logger m_log_test;
    boost::program_options::variables_map m_variable_map;
};
