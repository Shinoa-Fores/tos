#pragma once

// Shuts down the loader
void ShutdownTINE(int);
// Gets command line boot text that will be executed
// after boot, basically AUTOEXEC.BAT in text form
auto CmdLineBootText() -> char const*;

// operating in cmd line mode?
auto IsCmdLine() -> u64;

// used in sdl_window.cxx for SDL_CondWaitTimeout
auto GetFPS() -> int;

// vim: set expandtab ts=2 sw=2 :
