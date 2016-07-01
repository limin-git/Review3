#include "stdafx.h"
#include "Log.h"
#include "OptionString.h"
#include "ReviewManager.h"
#include "OptionString.h"
#include "ProgramOptions.h"
#include "ConsoleCommand.h"
#include "Utility.h"


int _tmain(int argc, _TCHAR* argv[])
{
    namespace op = boost::program_options;
    op::options_description desc( "Options", 100 );
    desc.add_options()
        ( "help,?", "produce help message" )
        ( file_name_option, op::wvalue<std::wstring>(),  "the file to be reviewed" )
        ( file_history_option, op::wvalue<std::wstring>(),  ".history" )
        ( file_review_option, op::wvalue<std::wstring>(),  ".review, history cache" )
        ( config_option, op::wvalue<std::wstring>(),  "config file" )
        ( review_schedule, op::wvalue<std::wstring>(), "review schedule (time span list)" )
        ( review_minimal_time_option, op::value<boost::timer::nanosecond_type>()->default_value( 500 ),  "in miniseconds" )
        ( review_max_cache_size_option, op::value<size_t>()->default_value( 100 ),  "normally write to .review, write to .history every max-cache-size times" )
        ( review_auto_update_interval_option, op::value<size_t>()->default_value( 60 ),  "in seconds" )
        ( review_order_option, op::wvalue<std::wstring>(),  "review order in format of '<latest>-<earlist>-<random>" )
        ( review_display_format_option, op::wvalue<std::wstring>(),  "display format" )
        ( review_once_per_days_option, op::value<size_t>()->default_value( 0 ),  "only review once in [n] days" )
        ( review_minimal_review_distance_option, op::value<size_t>()->default_value( 10 ),  "minimal review distance" )
        ( speech_play_back, op::value<size_t>()->default_value( 0 ),  "listen back [n]" )
        ( speech_disabled_option, op::wvalue<std::wstring>(), "true|false" )
        ( speech_path_option, op::wvalue< std::vector<std::wstring> >()->multitoken(), "speech path" )
        ( speech_no_duplicate, op::wvalue<std::wstring>(), "no duplicate (true|false)" )
        ( speech_no_text_to_speech, op::wvalue<std::wstring>(), "no TTS(text-to-speech) (true|false)" )
        ( listen_no_string_option, op::wvalue<std::wstring>(), "no original string (true|false)" )
        ( listen_all_option, op::wvalue<std::wstring>(), "listen all? (true|false)" )
        ( upgrade_hash_algorithm_option, op::wvalue<std::wstring>(), "upgrade hash algorithm (true|false)" )
        ( system_font_face_name, op::wvalue<std::wstring>(), "console font name" )
        ( system_font_size, op::value<SHORT>()->default_value( 18 ), "console font size" )
        ( system_console_width, op::value<SHORT>()->default_value( 60 ), "console width" )
        ( system_console_height, op::value<SHORT>()->default_value( 5 ), "console height" )
        ( system_console_color, op::wvalue<std::wstring>(), "console color" )
        ( system_picture_path, op::wvalue<std::wstring>(), "desktop wallpaper path" )
        ;

    desc.add( Log::get_description() );
    ProgramOptions::initialize( argc, argv, desc );
    const op::variables_map& vm = ProgramOptions::get_vm();

    if ( vm.count( "help" ) )
    {
        std::cout << desc << std::endl;
        return 0;
    }

    Log::initialize( vm );

    if ( ! vm.count( file_name_option ) )
    {
        std::cout << "must set " << file_name_option << std::endl;
        std::cout << desc << std::endl;
        return 0;
    }

    std::wstring file_name = vm[file_name_option].as<std::wstring>();
    Utility::remove_if_isany_of( file_name, ".\\|/:<>?*\"" );

    if ( ::CreateMutex( NULL, FALSE, file_name.c_str() ) == NULL || GetLastError() == ERROR_ALREADY_EXISTS )
    {
        std::cout << "error: another instance is running." << std::endl;
        system( "pause" );
        return 0;
    }

    ConsoleCommand console_command;

    try
    {
        ::CoInitializeEx( NULL, COINIT_MULTITHREADED );
        ReviewManager rm;

        //if ( vm[upgrade_hash_algorithm_option].as<std::wstring>() == L"true" )
        //{
        //    rm.upgrade_hash_algorithm();
        //    return 0;
        //}

        rm.review();
    }
    catch ( boost::filesystem::filesystem_error& e )
    {
        std::cout << e.what() << std::endl;
        system( "pause" );
    }
    catch ( std::exception& e )
    {
        std::cout << e.what() << std::endl;
        system( "pause" );
    }
    catch ( ... )
    {
        std::cout << "caught exception, exit." << std::endl;
        system( "pause" );
    }

	return 0;
}
