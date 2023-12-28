#pragma once

#ifdef _WIN32
  #include <winsock2.h>
#endif // stop fucking whining about winsock holy shit

#include "pch.h"

#include "../vendor/fmt/include/fmt/core.h"
#include "../vendor/fmt/include/fmt/format.h"
#include <algorithm>
#include <array>
#include <bit>
#include <chrono>
#include <filesystem>
#include <functional>
#include <initializer_list>
#include <limits>
#include <new>
#include <string>
#include <string_view>
#include <system_error>
#include <unordered_map>
#include <utility>
#include <vector>
