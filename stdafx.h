// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#include <stdio.h>
#include <tchar.h>



// TODO: reference additional headers your program requires here
#include <windows.h>
#include <MMSystem.h>
#pragma  comment( lib, "winmm.lib" )
#pragma  comment( lib, "Psapi.lib" )
#include <dshow.h>
#pragma comment(lib, "strmiids.lib") // For IID_IGraphBuilder, IID_IMediaControl, IID_IMediaEvent
#include <atlbase.h>
#include <atlcom.h>
#include <sapi.h>
#include <stdlib.h>
#include <conio.h>
#include <limits>
#include <iostream>
#include <string>
#include <vector>
#include <iomanip>
#include <algorithm>
#include <map>
#include <set>
#include <list>
#include <fstream>
#include <sstream>
#include <boost/regex.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/program_options.hpp>
#include <boost/signals2.hpp>
#include <boost/log/sources/logger.hpp>
#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/log/utility/setup.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/random_generator.hpp>
#include <boost/functional.hpp>
#include <boost/functional/hash.hpp>
#include <boost/local_function.hpp>
#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/lock_guard.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/timer/timer.hpp>
#include <boost/chrono/io/duration_io.hpp>
#include <boost/locale/encoding_utf.hpp>
#include <boost/locale/encoding.hpp>
#include <boost/any.hpp>
#include <boost/variant.hpp>
#include <boost/locale.hpp>
