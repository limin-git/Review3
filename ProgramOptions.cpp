#include "stdafx.h"
#include "ProgramOptions.h"
#include "Log.h"
#include "Utility.h"
#include "OptionString.h"
#include "DirectoryWatcher.h"


std::wstring ProgramOptions::m_config_file;
std::time_t ProgramOptions::m_last_write_time;
signal_type ProgramOptions::m_signal;
boost::mutex ProgramOptions::m_mutex;
boost::program_options::variables_map ProgramOptions::m_vm;
const boost::program_options::options_description* ProgramOptions::m_desc;


void ProgramOptions::initialize( int argc, wchar_t* argv[], const boost::program_options::options_description& desc )
{
    try
    {
        m_desc = &desc;

        store( boost::program_options::wcommand_line_parser(argc, argv).options(desc).allow_unregistered().run(), m_vm );
        notify( m_vm );

#if 0
        if ( m_vm.count( config_option ) )
        {
            m_config_file = m_vm[config_option].as<std::wstring>();

            if ( boost::filesystem::exists( m_config_file ) )
            {
                store( boost::program_options::parse_config_file<wchar_t>( std::wifstream( m_config_file.c_str() ), desc, true ), m_vm );
                notify( m_vm );
                m_last_write_time = boost::filesystem::last_write_time( m_config_file );
            }

            DirectoryWatcher::connect_to_signal( boost::bind( &ProgramOptions::process_config_file_change ), m_config_file );
        }
#endif
        return;
    }
    catch ( boost::filesystem::filesystem_error& e )
    {
        std::cout << e.what() << std::endl;
    }
    catch ( std::exception& e )
    {
        std::cout << e.what() << std::endl;
    }
    catch ( ... )
    {
        std::cout << "caught exception, exit." << std::endl;
    }

    ::exit( 0 );
}


boost::signals2::connection  ProgramOptions::connect_to_signal( slot_type slot )
{
    slot( m_vm );
    return m_signal.connect( slot );
}


void ProgramOptions::process_config_file_change()
{
    boost::unique_lock<boost::mutex> lock( m_mutex );

    try
    {
        std::time_t t = boost::filesystem::last_write_time( m_config_file );

        if ( t == m_last_write_time )
        {
            return;
        }

        LOG_DEBUG << m_config_file;

        m_vm.clear();
        store( boost::program_options::parse_config_file<wchar_t>( std::wifstream( m_config_file.c_str() ), *m_desc, true ), m_vm );
        notify( m_vm );
        m_signal( m_vm );
        m_last_write_time = t;
    }
    catch ( boost::filesystem::filesystem_error& e)
    {
        LOG << e.what();
    }
    catch ( std::exception& e)
    {
        LOG << e.what();
    }
}
