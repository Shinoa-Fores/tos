#include "tosprint.hxx"

namespace {
auto UnescapeString(char* str, char* where) -> char* {
  while (*str) {
    char const* to;
    switch (*str) {
#define ESC(c, e) \
  case c:         \
    to = e;       \
    break
      ESC('\\', "\\\\");
      ESC('\a', "\\a");
      ESC('\b', "\\b");
      ESC('\f', "\\f");
      ESC('\n', "\\n");
      ESC('\r', "\\r");
      ESC('\t', "\\t");
      ESC('\v', "\\v");
      ESC('\"', "\\\"");
    default:
      goto check_us_key;
    }
    // word
    memcpy(where, to, 2);
    where += 2;
    ++str;
    continue;
  check_us_key:
    if (!isalnum(*str) && !strchr(" ~!@#$%^&*()_+|{}[]\\;':\",./<>?", *str)) {
      fmt::format_to_n(where, 4, "\\{:o}", static_cast<u8>(*str));
      where += 4;
      ++str;
      continue;
    }
    // default when matches none of the criteria above
    *where++ = *str++;
  }
  *where = 0;
  return where;
}

auto MStrPrint(char const* fmt, u64, i64* argv) -> std::string {
  // this does not compare argument count(argc)
  // with StrOcc(fmt, '%'), be careful i guess
  // it also isn't a fully featured one but should
  // account for most use cases
  char const *start = fmt, *end;
  i64         arg   = -1;
  //
  std::string ret;
  ret.reserve(128);
  char buf[64];
  while (true) {
    ++arg;
    end = strchr(start, '%');
    if (!end)
      end = start + strlen(start);
    ret += std::string_view{start, end};
    if (!*end)
      return ret;
    start = end + 1;
    if (*start == '-')
      ++start;
    /* this skips output format specifiers
     * because i dont think a debug printer
     * needs such a thing */
    // i64 width = 0, decimals = 0;
    while (isdigit(*start)) {
      // width *= 10;
      // width += *start - '0';
      ++start;
    }
    if (*start == '.') {
      ++start;
      while (isdigit(*start)) {
        // decimals *= 10;
        // decimals += *start - '0';
        ++start;
      }
    }
    while (strchr("t,$/", *start))
      ++start;
    i64 aux = 1;
    if (*start == '*') {
      aux = argv[arg++];
      ++start;
    } else if (*start == 'h') {
      while (isdigit(*start)) {
        aux *= 10;
        aux += *start - '0';
        ++start;
      }
    }
    // clang-format off
#define FMT_TYP(fmt_lit, T)                     \
  ret.append(buf, fmt::format_to_n(             \
                    buf, sizeof buf, fmt_lit,   \
                    std::bit_cast<T>(argv[arg]) \
                  ).out - buf);
    // clang-format on
    switch (*start) {
    case 'd':
    case 'i':
      FMT_TYP("{}", i64);
      break;
    case 'u':
      FMT_TYP("{}", u64);
      break;
    case 'x':
      FMT_TYP("{:x}", u64);
      break;
    case 'n':
      // 7: ansf precision in TempleOS
      FMT_TYP("{:.7f}", f64);
      break;
    case 'p':
      FMT_TYP("{}", void*);
      break;
    case 'c':
      // this accounts for HolyC's multichar character literals too
      // null-terminated because multichar(variable size albeit U64)
      while (--aux >= 0) {
        union {
          char s[9]{};
          u64  i;
        } u;
        u.i = {((u64*)argv)[arg]};
        ret += u.s;
      }
      break;
    case 's':
      while (--aux >= 0)
        ret += ((char**)argv)[arg];
      break;
    case 'q': {
      auto str = ((char**)argv)[arg];
      // It's a VLA, I know
      char esc_buf[strlen(str) * 4 + 1];
      UnescapeString(str, esc_buf);
      ret += esc_buf;
    } break;
    case '%':
      ret += '%';
      break;
    }
    ++start;
  }
}
} // namespace

void TOSPrint(char const* fmt, u64 argc, i64* argv) {
  fmt::print(stderr, "{}", MStrPrint(fmt, argc, argv));
  fflush(stderr);
}

// vim: set expandtab ts=2 sw=2 :
