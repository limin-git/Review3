#pragma once
#include "Log.h"


class OptionUpdateHelper
{
public:

    template<typename T>
    bool update_one_option( const char* option_name, const boost::program_options::variables_map& vm, const T& default_value = T() )
    {
        try
        {
            T old_value = get_value<T>(option_name);

            if ( vm.count( option_name ) )
            {
                T new_value = vm[option_name].as<T>();

                if ( new_value != old_value )
                {
                     m_variables[option_name] = new_value;
                    return true;
                }
            }
            else
            {
                if ( old_value != default_value )
                {
                     m_variables[option_name] = default_value;
                    return true;
                }
            }
        }
        catch ( std::exception& e )
        {
            LOG << e.what();
        }
        catch ( ... )
        {
            LOG << "caught exception";
        }

        return false;
    }

    template<typename T> T get_value( const char* option_name )
    {
        try
        {
            boost::any option = m_variables[option_name];

            if ( ! option.empty() )
            {
                return boost::any_cast<T>( option );
            }
        }
        catch ( std::exception& e )
        {
            LOG << e.what();
        }
        catch ( ... )
        {
            LOG << "caught exception";
        }

        return T();
    }

public:

    std::map<std::string, boost::any> m_variables;
};
