//===-- sdl3_stubs.c - the SDL3 subsystems this port does not carry --------===//
//
// Two kinds of gap, both deliberate:
//
//  - A subsystem implemented in C++ (GameInput), which this backend cannot
//    take. The video driver calls its entry points unconditionally, so they
//    have to exist or WIN_VideoInit stops halfway.
//  - A subsystem whose implementation is a platform adapter that was never
//    compiled, e.g. notifications or async I/O.
//
// Everything listed here is called only on the paths this test does not
// exercise, so a no-op is faithful enough: the point of the fixture is the
// code that *does* run.
//
// The signatures must match SDL_windowsgameinput.h exactly. They are C, and
// the backend matches by name, so a wrong parameter count assembles and then
// fails at run time.
//===----------------------------------------------------------------------===//

#include <stdbool.h>

typedef struct SDL_VideoDevice SDL_VideoDevice;

// src/video/windows/SDL_windowsgameinput.h (C++)
__attribute__((used, externally_visible)) bool WIN_InitGameInput(
    SDL_VideoDevice *device) {
  (void)device;
  return false;
}
__attribute__((used, externally_visible)) bool WIN_UpdateGameInputEnabled(
    SDL_VideoDevice *device) {
  (void)device;
  return false;
}
__attribute__((used, externally_visible)) void WIN_UpdateGameInput(
    SDL_VideoDevice *device) {
  (void)device;
}
__attribute__((used, externally_visible)) void WIN_QuitGameInput(
    SDL_VideoDevice *device) {
  (void)device;
}

// src/notification/SDL_notification.c reaches a platform backend this port
// does not build.
__attribute__((used, externally_visible)) void SDL_CleanupNotifications(void) {}

// src/async/SDL_asyncio.c's platform half.
__attribute__((used, externally_visible)) void SDL_SYS_QuitAsyncIO(void) {}
