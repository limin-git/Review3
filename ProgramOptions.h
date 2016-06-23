#pragma once
typedef boost::signals2::signal<void (const boost::program_options::variables_map&)> signal_type;
typedef signal_type::slot_type slot_type;


class ProgramOptions
{
public:

    static void initialize( int argc, char* argv[], const boost::program_options::options_description& desc );
    static boost::signals2::connection connect_to_signal( slot_type slot );
    static const boost::program_options::variables_map& get_vm() { return m_vm; }

public:

    static void process_config_file_change(); // DirectoryWatcher slot

public:

    static boost::mutex m_mutex;
    static std::string m_config_file;
    static std::time_t m_last_write_time;
    static signal_type m_signal;
    static boost::program_options::variables_map m_vm;
    static const boost::program_options::options_description* m_desc;
};
