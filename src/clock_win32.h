#pragma once
#if defined(INCLUDE_CLOCK_WIN32) && INCLUDE_CLOCK_WIN32

#include <time.h>

// Copied from c:/Rtools/mingw_32/i686-w64-mingw32/include/pthread_time.h
#ifndef CLOCK_REALTIME
#define CLOCK_REALTIME              0
#endif

int clock_gettime(int X, struct timeval *tv);

#endif
