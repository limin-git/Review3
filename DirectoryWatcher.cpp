#include "stdafx.h"
#include "DirectoryWatcher.h"
#include "Log.h"


std::map<std::wstring, DirectoryWatcher::signal_type*> DirectoryWatcher::m_signals;


void DirectoryWatcher::connect_to_signal( slot_type slot, const std::wstring& file )
{
    m_signals[file] = new signal_type;
    m_signals[file]->connect( slot );
    boost::thread( boost::bind( &DirectoryWatcher::watch_directory_thread, file ) );
}


void DirectoryWatcher::watch_directory_thread( const std::wstring& file )
{
    if ( ! boost::filesystem::exists( file ) )
    {
        LOG << "can not find: " << file;
        return;
    }

    if ( ! boost::filesystem::is_regular_file( file ) )
    {
        LOG << "it is not a file: " << file;
        return;
    }

    std::wstring directory = boost::filesystem::system_complete( file ).parent_path().wstring();
    HANDLE handle = ::FindFirstChangeNotification( directory.c_str(), FALSE, FILE_NOTIFY_CHANGE_LAST_WRITE );

    if ( handle == INVALID_HANDLE_VALUE )
    {
        LOG << file << ", FindFirstChangeNotification error: " << ::GetLastError();
        return;
    }

    std::time_t last_write_time = boost::filesystem::last_write_time( file );

    while ( true )
    {
        DWORD status = ::WaitForSingleObject( handle, INFINITE );

        if ( WAIT_OBJECT_0 == status )
        {
            std::time_t t = boost::filesystem::last_write_time( file );

            if ( t != last_write_time )
            {
                last_write_time = t;
                LOG_DEBUG << file;
                (*m_signals[file])();
            }

            if ( ::FindNextChangeNotification( handle ) == FALSE )
            {
                LOG << file << ", FindNextChangeNotification error: " << ::GetLastError();
                return;
            }
        }
        else
        {
            LOG << file << ", WaitForSingleObject error: " << ::GetLastError();
            return;
        }
    }
}
