#include "stdafx.h"
#include "Log.h"
#include "ReviewManager.h"
#include "OptionString.h"
#include "ProgramOptions.h"
#include "ConsoleCommand.h"
#include "Utility.h"


int main(int argc, char* argv[])
{
    boost::program_options::options_description desc( "Options", 100 );
    desc.add_options()
        ( "help,?", "produce help message" )
        ( file_name_option, boost::program_options::value<std::string>(),  "the file to be reviewed" )
        ( file_history_option, boost::program_options::value<std::string>(),  ".history" )
        ( file_review_option, boost::program_options::value<std::string>(),  ".review, history cache" )
        ( config_option, boost::program_options::value<std::string>()->default_value( "review.cfg" ),  "config file" )
        ( review_schedule, boost::program_options::value<std::string>(), "review schedule (time span list)" )
        ( review_minimal_time_option, boost::program_options::value<boost::timer::nanosecond_type>()->default_value( 500 ),  "in miniseconds" )
        ( review_max_cache_size_option, boost::program_options::value<size_t>()->default_value( 100 ),  "normally write to .review, write to .history every max-cache-size times" )
        ( review_auto_update_interval_option, boost::program_options::value<size_t>()->default_value( 60 ),  "in seconds" )
        ( review_order_option, boost::program_options::value<std::string>()->default_value( "Latest" ),  "review order in format of '<latest>-<earlist>-<random>" )
        ( review_display_format_option, boost::program_options::value<std::string>()->default_value( "Q,AE" ),  "display format" )
        ( review_once_per_days_option, boost::program_options::value<size_t>()->default_value( 0 ),  "only review once in [n] days" )
        ( review_minimal_review_distance_option, boost::program_options::value<size_t>()->default_value( 10 ),  "minimal review distance" )
        ( speech_play_back, boost::program_options::value<size_t>()->default_value( 0 ),  "listen back [n]" )
        ( speech_disabled_option, boost::program_options::value<std::string>()->default_value("false"), "true|false" )
        ( speech_path_option, boost::program_options::value< std::vector<std::string> >()->multitoken(), "speech path" )
        ( speech_no_duplicate, boost::program_options::value<std::string>()->default_value( "false" ), "no duplicate (true|false)" )
        ( speech_no_text_to_speech, boost::program_options::value<std::string>()->default_value( "false" ), "no TTS(text-to-speech) (true|false)" )
        ( listen_no_string_option, boost::program_options::value<std::string>()->default_value( "false" ), "no original string (true|false)" )
        ( listen_all_option, boost::program_options::value<std::string>()->default_value( "false" ), "listen all? (true|false)" )
        ( upgrade_hash_algorithm_option, boost::program_options::value<std::string>()->default_value( "false" ), "upgrade hash algorithm (true|false)" )
        ( system_font_face_name, boost::program_options::value<std::string>()->default_value( "ÐÂËÎÌå" ), "console font name" )
        ( system_font_size, boost::program_options::value<SHORT>()->default_value( 18 ), "console font size" )
        ( system_console_width, boost::program_options::value<SHORT>()->default_value( 60 ), "console width" )
        ( system_console_height, boost::program_options::value<SHORT>()->default_value( 5 ), "console height" )
        ( system_console_color, boost::program_options::value<std::string>()->default_value( "0x7F" ), "console color" )
        ( system_picture_path, boost::program_options::value<std::string>(), "desktop wallpaper path" )
        ;

    desc.add( Log::get_description() );
    ProgramOptions::initialize( argc, argv, desc );
    const boost::program_options::variables_map& vm = ProgramOptions::get_vm();

    if ( vm.count( "help" ) )
    {
        std::cout << desc << std::endl;
        return 0;
    }

    Log::initialize( vm );

    if ( ! vm.count( file_name_option ) )
    {
        std::cout << "must set " << file_name_option;
        std::cout << desc << std::endl;
        return 0;
    }

    std::string file_name = vm[file_name_option].as<std::string>();
    Utility::remove_if_isany_of( file_name, ".\\|/:<>?*\"" );

    if ( ::CreateMutex( NULL, FALSE, file_name.c_str() ) == NULL || GetLastError() == ERROR_ALREADY_EXISTS )
    {
        std::cout << "another instance is running." << std::endl;
        system( "pause" );
        return 0;
    }

    ConsoleCommand console_command;

    try
    {
        ::CoInitializeEx( NULL, COINIT_MULTITHREADED );
        ReviewManager rm;

        if ( vm[upgrade_hash_algorithm_option].as<std::string>() == "true" )
        {
            rm.upgrade_hash_algorithm();
            return 0;
        }

        rm.review();
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

	return 0;
}
