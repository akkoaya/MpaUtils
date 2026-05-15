#pragma once

#define BOOST_PROCESS_USE_STD_FS 1

#include <boost/asio.hpp>
#include <boost/process/v1.hpp>
#ifdef _WIN32
#include <boost/process/v1/extend.hpp>
#include <boost/process/v1/windows.hpp>
#endif
