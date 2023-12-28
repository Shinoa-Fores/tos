#include "backtrace.hxx"
#include "tos_aot.hxx"

namespace {
std::vector<std::string> sorted_syms;
std::string const        unknown_fun{"UNKNOWN"};

void InitSortedSyms() {
  static bool init = false;
  if (init)
    return;
  sorted_syms.reserve(TOSLoader.size());
  for (auto const& [name, _] : TOSLoader)
    sorted_syms.emplace_back(name);
  std::sort(sorted_syms.begin(), sorted_syms.end(),
            [](auto const& a, auto const& b) -> bool {
              return TOSLoader[a].val < TOSLoader[b].val;
            });
  init = true;
}

} // namespace

void BackTrace(uptr ctx_rbp, uptr ctx_rip) {
  InitSortedSyms();
  fputc('\n', stderr);
  auto  rbp = reinterpret_cast<void*>(ctx_rbp);
  auto  ptr = reinterpret_cast<void*>(ctx_rip);
  void* oldp;
  auto  last = std::cref(unknown_fun);
  while (rbp) {
    oldp = nullptr;
    last = unknown_fun;
    // linear search that iterates over symbols sorted in ascending address
    // order to find out where we were
    for (auto const& s : sorted_syms) {
      void* curp = TOSLoader[s].val;
      if (curp == ptr) {
        fmt::print(stderr, "{} [{}]", s, ptr);
      } else if (curp > ptr) {
        fmt::print(stderr, "{} [{}+{:#x}] {}\n", last.get(), ptr,
                   (u8*)ptr - (u8*)oldp, curp);
        goto next;
      }
      oldp = curp;
      last = s;
    }
  next:
    ptr = static_cast<void**>(rbp)[1]; // [RBP+0x8] is the return address
    rbp = static_cast<void**>(rbp)[0]; // [RBP] is the previous base pointer
  }
  fputc('\n', stderr);
}

// will not fix minute memory leaks here,
// its gonna be executed once or twice in
// the entire debug session not to mention
// WhichFun() wont even be called in normal
// circumstances
#define STR_DUP(s)                                              \
  ({                                                            \
    auto const& s_ = s.get();                                   \
    strcpy(new (std::nothrow) char[s_.size() + 1], s_.c_str()); \
  })

// great when you use gdb and get a fault
// (gdb) p (char*)WhichFun($pc)
[[using gnu: used, visibility("default")]] auto WhichFun(void* ptr) -> char* {
  InitSortedSyms();
  auto last = std::cref(unknown_fun);
  for (auto const& s : sorted_syms) {
    if (TOSLoader[s].val >= ptr)
      return STR_DUP(last);
    last = s;
  }
  return STR_DUP(last);
}

// vim: set expandtab ts=2 sw=2 :
