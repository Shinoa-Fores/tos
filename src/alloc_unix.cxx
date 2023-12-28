#include "alloc.hxx"

namespace {
// used attribute for FreeBSD
[[gnu::used]] inline auto Hex2U64(char* ptr, char** res) -> u64 {
  u64  ret = 0;
  char c;
  while (isxdigit(c = *ptr)) {
    ret <<= 4;
    ret |= isalpha(c) ? toupper(c) - 'A' + 10 : c - '0';
    ++ptr;
  }
  *res = ptr;
  return ret;
}

usize page_size;
} // namespace

auto NewVirtualChunk(usize sz, bool low32) -> void* {
  static bool first_run = true;
  if (first_run) {
    page_size = sysconf(_SC_PAGESIZE);
    first_run = false;
  }
  // page_size is a power of 2 so this works, see explanation on win32 source
  usize padded_sz = (sz + page_size - 1) & ~(page_size - 1);
  void* ret;
  if (low32) { // code heap
    ret = mmap(nullptr, padded_sz, PROT_EXEC | PROT_WRITE | PROT_READ,
               MAP_PRIVATE | MAP_ANON | MAP_32BIT, -1, 0);
#ifdef __linux__ // parse memory maps for allocation gaps that mmap missed
    if (ret == MAP_FAILED) {
      // clang-format off
      // side note: Linux doesn't seem to like allocating stuff below 31 bits (<0x40000000)
      // I don't know why so technically we have 1GB less space for the code heap than on
      // Windows or FreeBSD but it won't really matter since machine code doesn't take up a lot of space
      // clang-format on
      int fd = open("/proc/self/maps", O_RDONLY);
      if (fd == -1) [[unlikely]]
        return nullptr; // never happens
      char buf[BUFSIZ];
      // base address of allocation gap/region
      uptr    base       = 0;
      ssize_t read_bytes = 0;
      while ((read_bytes = read(fd, buf, BUFSIZ)) > 0) {
        ssize_t cur_pos = 0;
        while (cur_pos < read_bytes) {
          auto ptr = buf + cur_pos; // number on the left -> ?????-?????
                                    // top address of allocation gap
          u64 lower = Hex2U64(ptr, &ptr);
          // MAP_FIXED wants us to align `base` to the page size
          base = (base + page_size - 1) & ~(page_size - 1);
          // is gap size >= required allocation size?
          // && base must not be 0(first iteration)
          if (base && lower - base >= padded_sz)
            goto found;
          // ignore '-'
          // cat /proc/self/maps for an explanation
          ++ptr;
          base = Hex2U64(ptr, &ptr); // ?????-????? <- number on the right
                                     // base of allocation gap
          while (cur_pos < read_bytes && buf[cur_pos] != '\n')
            ++cur_pos;
          ++cur_pos; // skip newline
        }
      }
    found:
      close(fd);
      if (base > std::numeric_limits<u32>::max())
        return nullptr;
      ret = mmap((void*)base, padded_sz, PROT_EXEC | PROT_WRITE | PROT_READ,
                 MAP_PRIVATE | MAP_ANON | MAP_FIXED, -1, 0);
    } else
      return ret;
#endif
  } else // data heap
    ret = mmap(nullptr, padded_sz, PROT_WRITE | PROT_READ,
               MAP_PRIVATE | MAP_ANON, -1, 0);
  if (ret == MAP_FAILED)
    return nullptr;
  return ret;
}

void FreeVirtualChunk(void* ptr, usize sz) {
  usize padded_sz = (sz + page_size - 1) & ~(page_size - 1);
  munmap(ptr, padded_sz);
}

// vim: set expandtab ts=2 sw=2 :
