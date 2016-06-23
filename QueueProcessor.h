#pragma once


template< typename T = std::string, typename C = std::vector<T> >
struct QueueProcessor
{
    QueueProcessor( boost::function<void (const C&)> callback )
        : m_item_list_callback( callback )
    {
        m_running = true;
        m_thread = boost::thread( boost::bind( &QueueProcessor::process_item_list_thread_fun, this ) );
    }

    QueueProcessor( boost::function<void (const T&)> callback )
        : m_item_callback( callback )
    {
        m_running = true;
        m_thread = boost::thread( boost::bind( &QueueProcessor::process_item_thread_fun, this ) );
    }

    QueueProcessor()
    {
        terminate();
    }

    void terminate()
    {
        m_running = false;
        m_condition.notify_one();
        m_thread.join();
    }

    void queue_item( const T& item )
    {
        boost::unique_lock<boost::mutex> lock( m_mutex );
        m_items.insert( m_items.end(), item );
        m_condition.notify_one();
    }

    template<typename U>
    void queue_items( const U& items )
    {
        if ( items.empty() )
        {
            return;
        }

        boost::unique_lock<boost::mutex> lock( m_mutex );
        m_items.insert( m_items.end(), items.begin(), items.end() );
        m_condition.notify_one();
    }

    void process_item_list_thread_fun()
    {
        while ( m_running )
        {
            try
            {
                C items = get_items();

                if ( m_running )
                {
                    m_item_list_callback( items );
                }
            }
            catch ( std::exception& e )
            {
                //LOG << e.what();
            }
            catch ( ... )
            {
                //LOG << "caught exception";
            }
        }
    }


    void process_item_thread_fun()
    {
        while ( m_running )
        {
            try
            {
                T item = get_item();

                if ( m_running )
                {
                    m_item_callback( item );
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
        }
    }

    C get_items()
    {
        C items;
        boost::unique_lock<boost::mutex> lock( m_mutex );

        while ( m_items.empty() )
        {
            m_condition.wait( lock );
        }

        m_items.swap( items );
        return items;
    }

    T get_item()
    {
        T item;
        boost::unique_lock<boost::mutex> lock( m_mutex );

        while ( m_items.empty() )
        {
            m_condition.wait( lock );
        }

        if ( m_items.empty() )
        {
            return T();
        }

        item = m_items.begin();
        m_items.erase( m_items.begin() );
        return item;
    }

public:

    C m_items;
    boost::thread m_thread;
    volatile bool m_running;
    boost::mutex m_mutex;
    boost::condition_variable m_condition;
    boost::function<void (const C&)> m_item_list_callback;
    boost::function<void (const T&)> m_item_callback;
};
