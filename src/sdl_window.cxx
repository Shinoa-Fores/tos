#include "sdl_window.hxx"
#include "holyc_routines.hxx"
#include "logo.hxx"
#include "main.hxx"
#include "sound.h"

namespace {

struct CDrawWindow {
  SDL_mutex*    screen_mutex;
  SDL_cond*     screen_done_cond;
  SDL_Window*   window;
  SDL_Palette*  palette;
  SDL_Surface*  surf;
  SDL_Renderer* rend;
  i32           sz_x, sz_y;
  i32           margin_x, margin_y;
  bool          ready = false;
  // somehow segfaults idk lmao im just gonna leak memory for a
  // microsecond fuck you
  /*~CDrawWindow() noexcept {
    if (!win_init)
      return;
    SDL_DestroyCond(screen_done_cond);
    SDL_DestroyMutex(screen_mutex);
    SDL_FreePalette(palette);
    SDL_FreeSurface(surf);
    SDL_DestroyRenderer(rend);
    SDL_DestroyWindow(window);
    SDL_Quit();
  }*/
} win;

void DrawWindowUpdateCB(u8* px) {
  SDL_LockSurface(win.surf);
  memcpy(win.surf->pixels, px, 640 * 480);
  SDL_UnlockSurface(win.surf);
  SDL_RenderClear(win.rend);
  int w, h, w2, h2, margin_x = 0, margin_y = 0;
  SDL_GetWindowSize(win.window, &w, &h);
  if (w > h) {
    h2       = w * 480. / 640;
    w2       = w;
    margin_y = (h - h2) / 2;
    if (h2 > h) {
      margin_y = 0;
      goto top_margin;
    }
  } else {
  top_margin:
    w2       = h * 640. / 480;
    h2       = h;
    margin_x = (w - w2) / 2;
  }
  SDL_Rect viewport{
      .x = win.margin_x = margin_x,
      .y = win.margin_y = margin_y,
      .w = win.sz_x = w2,
      .h = win.sz_y = h2,
  };
  auto texture = SDL_CreateTextureFromSurface(win.rend, win.surf);
  SDL_RenderCopy(win.rend, texture, nullptr, &viewport);
  SDL_RenderPresent(win.rend);
  SDL_DestroyTexture(texture);
  SDL_CondBroadcast(win.screen_done_cond);
}

void DrawWindowNewCB() {
  if (SDL_Init(SDL_INIT_VIDEO)) {
    fmt::print(stderr,
               "Failed to init SDL's video subsystem with the following "
               "message: \"{}\"\n",
               SDL_GetError());
    exit(1);
  }
  // wtf
  SDL_SetHintWithPriority(SDL_HINT_VIDEO_X11_NET_WM_BYPASS_COMPOSITOR, "0",
                          SDL_HINT_OVERRIDE);
  SDL_SetHintWithPriority(SDL_HINT_RENDER_SCALE_QUALITY, "linear",
                          SDL_HINT_OVERRIDE);
  win.screen_mutex     = SDL_CreateMutex();
  win.screen_done_cond = SDL_CreateCond();
  win.window           = SDL_CreateWindow("TINE Is Not an Emulator", //
                                          SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                          640, 480, SDL_WINDOW_RESIZABLE);
  auto icon = SDL_CreateRGBSurfaceWithFormat(0, TINELogo.width, TINELogo.height,
                                             8 * TINELogo.bytes_per_pixel,
                                             SDL_PIXELFORMAT_RGBA32);
  SDL_LockSurface(icon);
  memcpy(icon->pixels, TINELogo.pixel_data,
         TINELogo.width * TINELogo.height * TINELogo.bytes_per_pixel);
  SDL_UnlockSurface(icon);
  SDL_SetWindowIcon(win.window, icon);
  SDL_FreeSurface(icon);
  win.surf    = SDL_CreateRGBSurface(0, 640, 480, 8, 0, 0, 0, 0);
  win.palette = SDL_AllocPalette(256);
  SDL_SetSurfacePalette(win.surf, win.palette);
  SDL_SetWindowMinimumSize(win.window, 640, 480);
  win.rend     = SDL_CreateRenderer(win.window, -1, SDL_RENDERER_ACCELERATED);
  win.margin_y = win.margin_x = 0;
  win.sz_x                    = 640;
  win.sz_y                    = 480;
  win.ready                   = true;
  // let templeos manage the cursor
  SDL_ShowCursor(SDL_DISABLE);
}

enum : Sint32 {
  WINDOW_UPDATE,
  WINDOW_NEW,
  AUDIO_INIT,
};

enum : u8 {
  CH_CTRLA       = 0x01,
  CH_CTRLB       = 0x02,
  CH_CTRLC       = 0x03,
  CH_CTRLD       = 0x04,
  CH_CTRLE       = 0x05,
  CH_CTRLF       = 0x06,
  CH_CTRLG       = 0x07,
  CH_CTRLH       = 0x08,
  CH_CTRLI       = 0x09,
  CH_CTRLJ       = 0x0A,
  CH_CTRLK       = 0x0B,
  CH_CTRLL       = 0x0C,
  CH_CTRLM       = 0x0D,
  CH_CTRLN       = 0x0E,
  CH_CTRLO       = 0x0F,
  CH_CTRLP       = 0x10,
  CH_CTRLQ       = 0x11,
  CH_CTRLR       = 0x12,
  CH_CTRLS       = 0x13,
  CH_CTRLT       = 0x14,
  CH_CTRLU       = 0x15,
  CH_CTRLV       = 0x16,
  CH_CTRLW       = 0x17,
  CH_CTRLX       = 0x18,
  CH_CTRLY       = 0x19,
  CH_CTRLZ       = 0x1A,
  CH_CURSOR      = 0x05,
  CH_BACKSPACE   = 0x08,
  CH_ESC         = 0x1B,
  CH_SHIFT_ESC   = 0x1C,
  CH_SHIFT_SPACE = 0x1F,
  CH_SPACE       = 0x20,
};

// Scan code flags
enum : u8 {
  SCf_E0_PREFIX = 7,
  SCf_KEY_UP    = 8,
  SCf_SHIFT     = 9,
  SCf_CTRL      = 10,
  SCf_ALT       = 11,
  SCf_CAPS      = 12,
  SCf_NUM       = 13,
  SCf_SCROLL    = 14,
  SCf_NEW_KEY   = 15,
  SCf_MS_L_DOWN = 16,
  SCf_MS_R_DOWN = 17,
  SCf_DELETE    = 18,
  SCf_INS       = 19,
  SCf_NO_SHIFT  = 30,
  SCf_KEY_DESC  = 31,
};

enum : u32 {
  SCF_E0_PREFIX = 1u << SCf_E0_PREFIX,
  SCF_KEY_UP    = 1u << SCf_KEY_UP,
  SCF_SHIFT     = 1u << SCf_SHIFT,
  SCF_CTRL      = 1u << SCf_CTRL,
  SCF_ALT       = 1u << SCf_ALT,
  SCF_CAPS      = 1u << SCf_CAPS,
  SCF_NUM       = 1u << SCf_NUM,
  SCF_SCROLL    = 1u << SCf_SCROLL,
  SCF_NEW_KEY   = 1u << SCf_NEW_KEY,
  SCF_MS_L_DOWN = 1u << SCf_MS_L_DOWN,
  SCF_MS_R_DOWN = 1u << SCf_MS_R_DOWN,
  SCF_DELETE    = 1u << SCf_DELETE,
  SCF_INS       = 1u << SCf_INS,
  SCF_NO_SHIFT  = 1u << SCf_NO_SHIFT,
  SCF_KEY_DESC  = 1u << SCf_KEY_DESC,
};

// TempleOS places a 1 in bit 7 for
// keys with an E0 prefix.
// See $LK,"::/Doc/CharOverview.DD"$
// and
// $LK,"KbdHndlr",A="MN:KbdHndlr"$().
enum : u8 {
  SC_ESC          = 0x01,
  SC_BACKSPACE    = 0x0E,
  SC_TAB          = 0x0F,
  SC_ENTER        = 0x1C,
  SC_SHIFT        = 0x2A,
  SC_CTRL         = 0x1D,
  SC_ALT          = 0x38,
  SC_CAPS         = 0x3A,
  SC_NUM          = 0x45,
  SC_SCROLL       = 0x46,
  SC_CURSOR_UP    = 0x48,
  SC_CURSOR_DOWN  = 0x50,
  SC_CURSOR_LEFT  = 0x4B,
  SC_CURSOR_RIGHT = 0x4D,
  SC_PAGE_UP      = 0x49,
  SC_PAGE_DOWN    = 0x51,
  SC_HOME         = 0x47,
  SC_END          = 0x4F,
  SC_INS          = 0x52,
  SC_DELETE       = 0x53,
  SC_F1           = 0x3B,
  SC_F2           = 0x3C,
  SC_F3           = 0x3D,
  SC_F4           = 0x3E,
  SC_F5           = 0x3F,
  SC_F6           = 0x40,
  SC_F7           = 0x41,
  SC_F8           = 0x42,
  SC_F9           = 0x43,
  SC_F10          = 0x44,
  SC_F11          = 0x57,
  SC_F12          = 0x58,
  SC_PAUSE        = 0x61,
  SC_GUI          = 0xDB,
  SC_PRTSCRN1     = 0xAA,
  SC_PRTSCRN2     = 0xB7,
};

// this is templeos' keymap
u8 constexpr keymap[] = {
    0,   CH_ESC, '1',  '2', '3',  '4', '5', '6', '7', '8', '9', '0', '-',
    '=', '\b',   '\t', 'q', 'w',  'e', 'r', 't', 'y', 'u', 'i', 'o', 'p',
    '[', ']',    '\n', 0,   'a',  's', 'd', 'f', 'g', 'h', 'j', 'k', 'l',
    ';', '\'',   '`',  0,   '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',',
    '.', '/',    0,    '*', 0,    ' ', 0,   0,   0,   0,   0,   0,   0,
    0,   0,      0,    0,   0,    0,   0,   0,   0,   '-', 0,   '5', 0,
    '+', 0,      0,    0,   0,    0,   0,   0,   0,   0,   0,   0,
};

auto consteval k2sc(u8 ch) -> u64 {
  for (usize i = 0; i < sizeof keymap / sizeof keymap[0]; ++i)
    if (keymap[i] == ch)
      return i;
  __builtin_unreachable();
}

auto ScanKey(u64* sc, SDL_Event* ev) -> int {
  u64 mod = 0;
  switch (ev->type) {
  case SDL_KEYDOWN:
  ent:
    *sc = ev->key.keysym.scancode;
    if (ev->key.keysym.mod & (KMOD_LSHIFT | KMOD_RSHIFT))
      mod |= SCF_SHIFT;
    else
      mod |= SCF_NO_SHIFT;
    if (ev->key.keysym.mod & (KMOD_LCTRL | KMOD_RCTRL))
      mod |= SCF_CTRL;
    if (ev->key.keysym.mod & (KMOD_LALT | KMOD_RALT))
      mod |= SCF_ALT;
    if (ev->key.keysym.mod & KMOD_CAPS)
      mod |= SCF_CAPS;
    if (ev->key.keysym.mod & KMOD_NUM)
      mod |= SCF_NUM;
    if (ev->key.keysym.mod & KMOD_LGUI)
      mod |= SCF_MS_L_DOWN;
    if (ev->key.keysym.mod & KMOD_RGUI)
      mod |= SCF_MS_R_DOWN;
    switch (ev->key.keysym.scancode) {
    case SDL_SCANCODE_SPACE:
      return *sc = k2sc(' ') | mod;
    case SDL_SCANCODE_APOSTROPHE:
      return *sc = k2sc('\'') | mod;
    case SDL_SCANCODE_COMMA:
      return *sc = k2sc(',') | mod;
    case SDL_SCANCODE_KP_MINUS:
    case SDL_SCANCODE_MINUS:
      return *sc = k2sc('-') | mod;
    case SDL_SCANCODE_KP_PERIOD:
    case SDL_SCANCODE_PERIOD:
      return *sc = k2sc('.') | mod;
    case SDL_SCANCODE_GRAVE:
      return *sc = k2sc('`') | mod;
    case SDL_SCANCODE_KP_DIVIDE:
    case SDL_SCANCODE_SLASH:
      return *sc = k2sc('/') | mod;
    case SDL_SCANCODE_KP_0:
    case SDL_SCANCODE_0:
      return *sc = k2sc('0') | mod;
    case SDL_SCANCODE_KP_1:
    case SDL_SCANCODE_1:
      return *sc = k2sc('1') | mod;
    case SDL_SCANCODE_KP_2:
    case SDL_SCANCODE_2:
      return *sc = k2sc('2') | mod;
    case SDL_SCANCODE_KP_3:
    case SDL_SCANCODE_3:
      return *sc = k2sc('3') | mod;
    case SDL_SCANCODE_KP_4:
    case SDL_SCANCODE_4:
      return *sc = k2sc('4') | mod;
    case SDL_SCANCODE_KP_5:
    case SDL_SCANCODE_5:
      return *sc = k2sc('5') | mod;
    case SDL_SCANCODE_KP_6:
    case SDL_SCANCODE_6:
      return *sc = k2sc('6') | mod;
    case SDL_SCANCODE_KP_7:
    case SDL_SCANCODE_7:
      return *sc = k2sc('7') | mod;
    case SDL_SCANCODE_KP_8:
    case SDL_SCANCODE_8:
      return *sc = k2sc('8') | mod;
    case SDL_SCANCODE_KP_9:
    case SDL_SCANCODE_9:
      return *sc = k2sc('9') | mod;
    case SDL_SCANCODE_SEMICOLON:
      return *sc = k2sc(';') | mod;
    case SDL_SCANCODE_EQUALS:
      return *sc = k2sc('=') | mod;
    case SDL_SCANCODE_LEFTBRACKET:
      return *sc = k2sc('[') | mod;
    case SDL_SCANCODE_RIGHTBRACKET:
      return *sc = k2sc(']') | mod;
    case SDL_SCANCODE_BACKSLASH:
      return *sc = k2sc('\\') | mod;
    case SDL_SCANCODE_Q:
      return *sc = k2sc('q') | mod;
    case SDL_SCANCODE_W:
      return *sc = k2sc('w') | mod;
    case SDL_SCANCODE_E:
      return *sc = k2sc('e') | mod;
    case SDL_SCANCODE_R:
      return *sc = k2sc('r') | mod;
    case SDL_SCANCODE_T:
      return *sc = k2sc('t') | mod;
    case SDL_SCANCODE_Y:
      return *sc = k2sc('y') | mod;
    case SDL_SCANCODE_U:
      return *sc = k2sc('u') | mod;
    case SDL_SCANCODE_I:
      return *sc = k2sc('i') | mod;
    case SDL_SCANCODE_O:
      return *sc = k2sc('o') | mod;
    case SDL_SCANCODE_P:
      return *sc = k2sc('p') | mod;
    case SDL_SCANCODE_A:
      return *sc = k2sc('a') | mod;
    case SDL_SCANCODE_S:
      return *sc = k2sc('s') | mod;
    case SDL_SCANCODE_D:
      return *sc = k2sc('d') | mod;
    case SDL_SCANCODE_F:
      return *sc = k2sc('f') | mod;
    case SDL_SCANCODE_G:
      return *sc = k2sc('g') | mod;
    case SDL_SCANCODE_H:
      return *sc = k2sc('h') | mod;
    case SDL_SCANCODE_J:
      return *sc = k2sc('j') | mod;
    case SDL_SCANCODE_K:
      return *sc = k2sc('k') | mod;
    case SDL_SCANCODE_L:
      return *sc = k2sc('l') | mod;
    case SDL_SCANCODE_Z:
      return *sc = k2sc('z') | mod;
    case SDL_SCANCODE_X:
      return *sc = k2sc('x') | mod;
    case SDL_SCANCODE_C:
      return *sc = k2sc('c') | mod;
    case SDL_SCANCODE_V:
      return *sc = k2sc('v') | mod;
    case SDL_SCANCODE_B:
      return *sc = k2sc('b') | mod;
    case SDL_SCANCODE_N:
      return *sc = k2sc('n') | mod;
    case SDL_SCANCODE_M:
      return *sc = k2sc('m') | mod;
    case SDL_SCANCODE_KP_MULTIPLY:
      return *sc = k2sc('*') | mod;
    case SDL_SCANCODE_KP_PLUS:
      return *sc = k2sc('+') | mod;
    case SDL_SCANCODE_ESCAPE:
      *sc = mod | SC_ESC;
      return 1;
    case SDL_SCANCODE_BACKSPACE:
      *sc = mod | SC_BACKSPACE;
      return 1;
    case SDL_SCANCODE_TAB:
      *sc = mod | SC_TAB;
      return 1;
    case SDL_SCANCODE_KP_ENTER:
    case SDL_SCANCODE_RETURN:
      *sc = mod | SC_ENTER;
      return 1;
    case SDL_SCANCODE_LSHIFT:
    case SDL_SCANCODE_RSHIFT:
      *sc = mod | SC_SHIFT;
      return 1;
    case SDL_SCANCODE_LALT:
    case SDL_SCANCODE_RALT:
      *sc = mod | SC_ALT;
      return 1;
    case SDL_SCANCODE_LCTRL:
    case SDL_SCANCODE_RCTRL:
      *sc = mod | SC_CTRL;
      return 1;
    case SDL_SCANCODE_CAPSLOCK:
      *sc = mod | SC_CAPS;
      return 1;
    case SDL_SCANCODE_NUMLOCKCLEAR:
      *sc = mod | SC_NUM;
      return 1;
    case SDL_SCANCODE_SCROLLLOCK:
      *sc = mod | SC_SCROLL;
      return 1;
    case SDL_SCANCODE_DOWN:
      *sc = mod | SC_CURSOR_DOWN;
      return 1;
    case SDL_SCANCODE_UP:
      *sc = mod | SC_CURSOR_UP;
      return 1;
    case SDL_SCANCODE_RIGHT:
      *sc = mod | SC_CURSOR_RIGHT;
      return 1;
    case SDL_SCANCODE_LEFT:
      *sc = mod | SC_CURSOR_LEFT;
      return 1;
    case SDL_SCANCODE_PAGEDOWN:
      *sc = mod | SC_PAGE_DOWN;
      return 1;
    case SDL_SCANCODE_PAGEUP:
      *sc = mod | SC_PAGE_UP;
      return 1;
    case SDL_SCANCODE_HOME:
      *sc = mod | SC_HOME;
      return 1;
    case SDL_SCANCODE_END:
      *sc = mod | SC_END;
      return 1;
    case SDL_SCANCODE_INSERT:
      *sc = mod | SC_INS;
      return 1;
    case SDL_SCANCODE_DELETE:
      *sc = mod | SC_DELETE;
      return 1;
    case SDL_SCANCODE_APPLICATION:
    case SDL_SCANCODE_LGUI:
    case SDL_SCANCODE_RGUI:
      *sc = mod | SC_GUI;
      return 1;
    case SDL_SCANCODE_PRINTSCREEN:
      *sc = mod | SC_PRTSCRN1;
      return 1;
    case SDL_SCANCODE_PAUSE:
      *sc = mod | SC_PAUSE;
      return 1;
    case SDL_SCANCODE_F1 ... SDL_SCANCODE_F12:
      *sc = mod | (SC_F1 + (ev->key.keysym.scancode - SDL_SCANCODE_F1));
      return 1;
    default:;
    }
    break;
  case SDL_KEYUP:
    mod |= SCF_KEY_UP;
    goto ent;
  }
  return -1;
}

static void* kb_cb   = nullptr;
static bool  kb_init = false;

auto SDLCALL KBCallback(void*, SDL_Event* e) -> int {
  u64 sc;
  if (kb_cb && (-1 != ScanKey(&sc, e)))
    FFI_CALL_TOS_1(kb_cb, sc);
  return 0;
}

static void* ms_cb   = nullptr;
static bool  ms_init = false;

auto SDLCALL MSCallback(void*, SDL_Event* e) -> int {
  static i32 x, y;
  static int state;
  static int z;
  int        x2, y2;
  // return value is actually ignored
  if (!ms_cb)
    return 0;
  switch (e->type) {
  case SDL_MOUSEBUTTONDOWN:
    x = e->button.x, y = e->button.y;
    if (e->button.button == SDL_BUTTON_LEFT)
      state |= 1 << 1;
    else // right
      state |= 1;
    goto ent;
  case SDL_MOUSEBUTTONUP:
    x = e->button.x, y = e->button.y;
    if (e->button.button == SDL_BUTTON_LEFT)
      state &= ~(1 << 1);
    else // right
      state &= ~1;
    goto ent;
  case SDL_MOUSEWHEEL:
    z -= e->wheel.y; // ???, inverted
    goto ent;
  case SDL_MOUSEMOTION:
    x = e->motion.x, y = e->motion.y;
  ent:
    if (x < win.margin_x)
      x2 = 0;
    else if (x > win.margin_x + win.sz_x)
      x2 = 640 - 1; // -1 because zero-indexed
    else
      x2 = (x - win.margin_x) * 640. / win.sz_x;

    if (y < win.margin_y)
      y2 = 0;
    else if (y > win.margin_y + win.sz_y)
      y2 = 480 - 1; // -1 because zero-indexed
    else
      y2 = (y - win.margin_y) * 480. / win.sz_y;
    FFI_CALL_TOS_4(ms_cb, x2, y2, z, state);
  }
  return 0;
}

} // namespace

void EventLoop() {
  if (SDL_Init(SDL_INIT_EVENTS)) {
    fmt::print(stderr,
               "Failed to init SDL's events subsystem with the following "
               "message: \"{}\"\n",
               SDL_GetError());
    exit(1);
  }
  if (SDL_RegisterEvents(1) != SDL_USEREVENT) {
    fmt::print(stderr, "THIS SHOULD NEVER HAPPEN, SDL SAYS: \"{}\"",
               SDL_GetError());
    exit(1);
  }
  SDL_Event e;
  while (SDL_WaitEvent(&e)) {
    switch (e.type) {
    case SDL_QUIT:
      return;
    case SDL_USEREVENT:
      switch (e.user.code) {
      case WINDOW_UPDATE:
        DrawWindowUpdateCB(static_cast<u8*>(e.user.data1));
        break;
      case WINDOW_NEW:
        DrawWindowNewCB();
        break;
      case AUDIO_INIT:
        InitSound();
        break;
      }
    }
  }
}

void SetClipboard(char const* text) {
  SDL_SetClipboardText(text);
}

static std::array<char*, 100> clips;

auto ClipboardText(void*) -> char* {
  static usize pos = 0;
  //
  auto clip = SDL_GetClipboardText();
  if (!clip)
    return nullptr;
  clips[pos++] = clip;
  if (pos >= 100) {
    for (usize i = 0; i < 99; ++i)
      SDL_free(clips[i]);
    // move clips[99](current clipboard)
    // to clips[0] to be freed on the next iteration
    clips[0] = clips[99];
    pos      = 1;
  }
  return clip;
}

void DrawWindowUpdate(u8* px) {
  static int fps = 0;
  if (!fps) [[unlikely]]
    fps = GetFPS();
  // https://archive.md/yD5QL
  SDL_Event event{
      .user = {},
  };
  auto& u = event.user;
  u.type  = SDL_USEREVENT;
  u.code  = WINDOW_UPDATE;
  u.data1 = px;
  // push to event queue so EventLoop receives it and updates screen
  SDL_PushEvent(&event);
  SDL_LockMutex(win.screen_mutex);
  SDL_CondWaitTimeout(win.screen_done_cond, win.screen_mutex, 1000 / fps);
  // CondWaitTimeout unlocks the mutex for us
}

// We call this from HolyC, it launches an event to EventLoop() so that it
// launches the SDL window for us(SDL is very picky about which thread launches
// the window)
void DrawWindowNew() {
  SDL_Event event{
      .user = {},
  };
  auto& u = event.user;
  u.type  = SDL_USEREVENT;
  u.code  = WINDOW_NEW;
  SDL_PushEvent(&event);
  // Spin until it's safe to write to the framebuffer
  // won't be long so it's fine to spin here
  while (!win.ready)
    SDL_Delay(1);
}

void PCSpkInit() {
  SDL_Event event{
      .user = {},
  };
  auto& u = event.user;
  u.type  = SDL_USEREVENT;
  u.code  = AUDIO_INIT;
  SDL_PushEvent(&event);
}

void SetKBCallback(void* fptr) {
  kb_cb = fptr;
  if (kb_init)
    return;
  kb_init = true;
  SDL_AddEventWatch(KBCallback, nullptr);
}

void SetMSCallback(void* fptr) {
  ms_cb = fptr;
  if (ms_init)
    return;
  ms_init = true;
  SDL_AddEventWatch(MSCallback, nullptr);
}

void GrPaletteColorSet(u64 i, bgr_48 u) {
  // 0xffff is 100% so 0x7fff/0xffff would be about .50
  // this gets multiplied by 0xff to get 0x7f
  u8 b = u.b / (f64)0xffff * 0xff;
  u8 g = u.g / (f64)0xffff * 0xff;
  u8 r = u.r / (f64)0xffff * 0xff;
  //
  SDL_Color sdl_c{
      .r = r,
      .g = g,
      .b = b,
      .a = 0xff,
  };
  // set column
  for (int repeat = 0; repeat < 256 / 16; ++repeat)
    SDL_SetPaletteColors(win.palette, &sdl_c, i + repeat * 16, 1);
}

// vim: set expandtab ts=2 sw=2 :
