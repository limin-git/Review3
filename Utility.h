#pragma once
#include "TimeUtility.h"
#include "StringUtility.h"
#include "UnicodeUtility.h"
#include "SoundUtility.h"
#include "ConsoleUtility.h"
#include "FileUtility.h"
#include "WriteConsoleHelper.h"

#ifdef max
#undef max
#undef min
#endif


namespace Utility
{
    size_t random_number( size_t lo = std::numeric_limits <size_t>::min(), size_t hi = std::numeric_limits <size_t>::max() );
    void set_system_wallpaper( const std::wstring& picture );
    std::vector<std::wstring> get_files_of_directory( const std::wstring& dir );
    bool is_picture( const std::wstring& file_name );
    bool remove_file( const std::wstring& file_name );
    bool rename_file( const std::wstring& old_file_name, const std::wstring& new_file_name );
}
