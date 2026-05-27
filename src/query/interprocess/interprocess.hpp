#pragma once

#include <boost/interprocess/sync/named_mutex.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>

namespace bi = boost::interprocess;

static const char* kMutexName = "stdout_mutex";