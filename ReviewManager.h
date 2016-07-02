#pragma once
#include "ReviewString.h"
class Loader;
class History;
class Speech;


class ReviewManager
{
public:

    enum EReviewDirection{ Forward, Backward };
    enum EReviewOrder{ Latest, Earliest, Random, Middle, Invalid };

public:

    ReviewManager();
    ~ReviewManager();
    void review();
    void listen_thread();

public:

    ReviewString get_next();
    ReviewString get_previous();
    static std::wstring wait_user_interaction();
    void set_console_title();
    void update();
    void update_thread();
    void update_option( const boost::program_options::variables_map& vm ); // ProgramOptions slot

public:

    std::wostream& output_hash_list( std::wostream& os, const std::list<size_t>& l );
    std::wstring get_hash_list_string( const std::list<size_t>& l );

public:

    std::wstring get_new_expired_string( const std::set<size_t>& os,  const std::set<size_t>& ns );

public:

    size_t get_next_hash( std::list<size_t>& hash_list, EReviewOrder order );
    EReviewOrder get_next_order( std::vector<EReviewOrder>& orders, std::vector<EReviewOrder>::iterator& it );
    std::vector<EReviewOrder> convert_from_string( const std::wstring& order_string );

public:

    void upgrade_hash_algorithm();
    void show_next_picture( const std::wstring& path = L"" );

public:

    size_t m_play_back;
    std::list<ReviewString> m_play_back_string;
    std::list<ReviewString> m_review_group;
    size_t m_minimal_review_distance;
    volatile bool m_is_listening;
    boost::condition_variable m_condition;
    boost::mutex m_mutex;
    std::wstring m_file_name;
    std::wstring m_history_name;
    std::wstring m_display_format;
    Loader* m_loader;
    History* m_history;
    Speech* m_speech;
    Speech* m_speech_impl;
    std::set<size_t> m_all;
    std::set<size_t> m_reviewing_set;
    std::list<size_t> m_reviewing_list;
    std::list<size_t> m_listening_list;
    EReviewDirection m_review_mode;
    size_t m_backward_index;
    std::vector<size_t> m_review_history;
    boost::timer::nanosecond_type m_minimal_review_time;
    volatile size_t m_auto_update_interval;
    volatile ReviewString* m_current_reviewing;
    std::vector<EReviewOrder> m_review_orders;
    std::vector<EReviewOrder>::iterator m_review_order_it;
    volatile bool m_listen_no_string;
    volatile bool m_listen_all;
    size_t m_review_number;
    std::map<size_t, size_t> m_hash_number_map;
    boost::signals2::connection m_connection;
    boost::thread m_update_thread;
    bool m_running;
    std::wstring m_picture_path;
    std::wstring m_picture_recycle_path;
    boost::filesystem::recursive_directory_iterator m_picture_dir_it;
};
