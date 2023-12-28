#include "main.hxx"
#include "cpp2holyc.hxx"
#include "dbg.hxx"
#include "sdl_window.hxx"
#include "seth.hxx"
#include "sound.h"
#include "tos_aot.hxx"
#include "vfs.hxx"

namespace fs = std::filesystem;

namespace {

int glbl_fps = 30;

std::string boot_str;
std::string bin_path = "HCRT.BIN";

bool is_cmd_line = false;

} // namespace

auto IsCmdLine() -> u64 {
  return is_cmd_line;
}

auto GetFPS() -> int {
  return glbl_fps;
}

auto CmdLineBootText() -> char const* {
  return boot_str.c_str();
}

#ifdef _WIN32
[[noreturn]] static auto WINAPI CtrlCHandlerRoutine(DWORD) -> BOOL {
  #define S(x) x, lstrlenA(x)
  WriteConsoleA(GetStdHandle(STD_ERROR_HANDLE), S("User Abort.\n"), nullptr,
                nullptr);
  ExitProcess(ERROR_CONTROL_C_EXIT);
  _Exit(ERROR_CONTROL_C_EXIT);
  #undef S
  return TRUE;
}
#endif

#ifdef main
  #undef main
#endif // hack to ignore SDL's tyranny of taking over main().
       // never had an issue with doing this although they
       // claim to perform critical setup with SDL_main()

auto main(int argc, char** argv) -> int {
#ifndef _WIN32
  // https://archive.md/5cufN#selection-2369.223-2369.272
  // Hilarious how Linux manpages won't teach me anything
  // about why I got an EPERM when I raised rl.rlim_max
  struct rlimit rl;
  getrlimit(RLIMIT_NOFILE, &rl);
  rl.rlim_cur = rl.rlim_max;
  setrlimit(RLIMIT_NOFILE, &rl);
  signal(SIGINT, [](int) {
  #define S(x) x, strlen(x)
    write(2, S("User abort.\n"));
    _Exit(1);
  #undef S
  });
#else
  SetConsoleCtrlHandler(CtrlCHandlerRoutine, TRUE);
#endif
  // i wanted to use cli11 but i dont want exceptions in this codebase
  struct arg_lit * helpArg, *sixty_fps, *commandLineArg;
  struct arg_file *cmdLineFiles, *TDriveArg, *HCRTArg;
  struct arg_end*  argEnd;
  //
  void* argtable[] = {
      helpArg        = arg_lit0("h", "help", "Display this help message"),
      sixty_fps      = arg_lit0("6", "60fps", "Run in 60 fps mode."),
      commandLineArg = arg_lit0("c", "com", "Command line mode, cwd -> Z:/"),
      HCRTArg        = arg_file0("f", "file", nullptr, "Specify HolyC runtime"),
      TDriveArg      = arg_file0("t", "root", nullptr, "Specify boot folder"),
      cmdLineFiles   = arg_filen(nullptr, nullptr, "<files>", 0, 100,
                                 "Files that run on boot(cmdline mode specific)"),
      argEnd         = arg_end_(10),
  };
  int errs = arg_parse(argc, argv, argtable);
  if (helpArg->count > 0 || errs > 0 || !TDriveArg->count) {
    if (errs)
      arg_print_errors(stderr, argEnd, argv[0]);
    fmt::print(stderr, "Usage: {}", argv[0]);
    arg_print_syntaxv(stderr, argtable, "\n");
    arg_print_glossary_gnu(stderr, argtable);
    return 1;
  }
  if (std::error_code e; fs::exists(TDriveArg->filename[0], e)) {
    VFsMountDrive('T', TDriveArg->filename[0]);
  } else if (e) {
    fmt::print(stderr, "SYSTEM ERROR OCCURED: {}\n", e.message());
    return 1;
  } else {
    fmt::print(stderr, "{} DOES NOT EXIST\n", TDriveArg->filename[0]);
    return 1;
  }
  if (commandLineArg->count > 0) {
    VFsMountDrive('Z', ".");
    is_cmd_line = true;
  }
  if (is_cmd_line) {
    for (int i = 0; i < cmdLineFiles->count; ++i) {
      boot_str += "#include \"";
      boot_str += cmdLineFiles->filename[i];
      boot_str += "\";\n";
    }
#ifdef _WIN32
    std::replace(boot_str.begin(), boot_str.end(), '\\', '/');
#endif
  }
  if (sixty_fps->count) {
    glbl_fps = 60;
    boot_str += "SetFPS(60.);\n";
  }
  if (HCRTArg->count > 0)
    bin_path = HCRTArg->filename[0];
  if (std::error_code e; !fs::exists(bin_path, e)) {
    if (e)
      fmt::print(stderr, "SYSTEM ERROR OCCURED: {}\n", e.message());
    else
      fmt::print(stderr,
                 "{} DOES NOT EXIST, MAYBE YOU FORGOT TO BOOTSTRAP IT? REFER "
                 "TO README FOR GUIDANCE\n",
                 bin_path);
    return 1;
  }
  arg_freetable(argtable, sizeof argtable / sizeof argtable[0]);
  BootstrapLoader();
  CreateCore(LoadHCRT(bin_path));
  EventLoop();
}

// vim: set expandtab ts=2 sw=2 :
