#pragma once

#include "types.h"

// clang-format off
// Windows requires sys/stat to be included after sys/types
// fuck you bill gates
#include <sys/types.h>
#include <sys/stat.h>
// clang-format on

#ifndef _WIN32
  #include <fcntl.h>
  #include <pthread.h>
  #include <sys/mman.h>
  #include <sys/resource.h>
  #include <ucontext.h>
  #include <unistd.h>
  #ifdef __linux__
    #include <linux/futex.h>
    #include <sys/syscall.h>
  #elif defined(__FreeBSD__)
    #include <sys/umtx.h>
  #endif
#else
  #define NOMINMAX
  #include <winsock2.h>
  #include <windows.h>
  #include <winbase.h>
  #include <wincon.h>
  #include <winerror.h>
  #include <winnt.h>
  #include <errhandlingapi.h>
  #include <memoryapi.h>
  #include <processthreadsapi.h>
  #include <profileapi.h>
  #include <synchapi.h>
  #include <sysinfoapi.h>
  #include <timeapi.h>
  // for mingw
  // https://archive.md/HEZm2#selection-3667.0-3698.0
  #ifndef ERROR_CONTROL_C_EXIT
    #define ERROR_CONTROL_C_EXIT 0x23C
  #endif
#endif

#include <argtable3.h>
#include <dyad.h>
#include <linenoise-ng/linenoise.h>

#include <SDL.h>

#include <ctype.h>
#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <tos_callconv.h>
