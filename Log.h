#pragma once


struct Log
{
    static void initialize( const boost::program_options::variables_map& vm = boost::program_options::variables_map() );
    static boost::program_options::options_description get_description();
    static void update_option( const boost::program_options::variables_map& vm );
};


extern boost::log::sources::wlogger m_log;
extern boost::log::sources::wlogger m_log_error;
extern boost::log::sources::wlogger m_log_info;
extern boost::log::sources::wlogger m_log_debug;
extern boost::log::sources::wlogger m_log_trace;
extern boost::log::sources::wlogger m_log_test;


#define SOURCE_INFO __FUNCTION__ << L" - "
#define LOG         BOOST_LOG(m_log)        << SOURCE_INFO
#define LOG_ERROR   BOOST_LOG(m_log_error)  << SOURCE_INFO
#define LOG_INFO    BOOST_LOG(m_log_info)   << SOURCE_INFO
#define LOG_DEBUG   BOOST_LOG(m_log_debug)  << SOURCE_INFO
#define LOG_TRACE   BOOST_LOG(m_log_trace)  << SOURCE_INFO
#define LOG_TEST    BOOST_LOG(m_log_test)   << SOURCE_INFO
