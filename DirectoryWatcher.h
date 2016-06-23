#pragma once


class DirectoryWatcher
{
public:

    typedef boost::signals2::signal<void()> signal_type;
    typedef signal_type::slot_type slot_type;

public:

    static void connect_to_signal( slot_type slot, const std::wstring& file );
    static void watch_directory_thread( const std::wstring& file );

public:

    static std::map<std::wstring, signal_type*> m_signals;
};
