//===-- sdl_demo.c - an SDL2 program built by llvm-msil -------------------===//
//
// Translated to CIL and run on the CLR, so the demo is also a test of what the
// backend can express at a native boundary: pinvoke to a third-party DLL,
// native -> managed callbacks on SDL's own threads, function pointers from the
// GL proc loader, and 56-byte event structs.
//
// Every scene is drawn with the software renderer and read back with
// SDL_RenderReadPixels, so the output is a handful of checksummed lines that
// can be compared against a native build. A window is still opened (hidden by
// default), because the renderer needs one.
//
//   ./sdl_demo [--frames N] [--visible] [--gl]
//
// --gl additionally runs a GL 3.3 core context if the driver offers one.
//===----------------------------------------------------------------------===//

#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// The renderer refuses to run without a window, and a hidden window is the
// only way to keep the demo non-interactive on a machine with a display.
#define DEMO_W 320
#define DEMO_H 200

static int gFrames = 90;
static int gVisible = 0;
static int gWantGL = 0;

static int gTimerTicks = 0;

static int fail(const char *what) {
  printf("FAIL %s: %s\n", what, SDL_GetError());
  return 1;
}

// SDL runs this on its own thread and the CLR has to enter managed code from
// it, which is the part of the boundary worth proving.
static Uint32 onTimer(Uint32 interval, void *param) {
  (void)param;
  gTimerTicks++;
  return interval;
}

//--- scenes -----------------------------------------------------------------//

// A deterministic gradient plus two moving bars, so a frame checksum covers
// both per-pixel work and SDL's own blending.
static void drawScene(SDL_Renderer *r, int frame) {
  SDL_SetRenderDrawColor(r, 12, 16, 32, 255);
  SDL_RenderClear(r);
  for (int y = 0; y < DEMO_H; y += 4) {
    SDL_SetRenderDrawColor(r, (Uint8)(y / 2), (Uint8)(frame & 0xff), 64, 255);
    SDL_Rect line = {0, y, DEMO_W, 2};
    SDL_RenderFillRect(r, &line);
  }
  for (int i = 0; i < 3; i++) {
    SDL_Rect box = {(frame * 3 + i * 60) % (DEMO_W - 40), 20 + i * 55, 40, 30};
    SDL_SetRenderDrawColor(r, (Uint8)(80 + i * 60), 200 - (Uint8)(i * 50), 40,
                           255);
    SDL_RenderFillRect(r, &box);
  }
}

// Two textures blended together, which exercises the texture path rather than
// the rect fills above.
static int drawTextures(SDL_Renderer *r, int frame) {
  const int W = 64, H = 64;
  Uint32 *pixels = (Uint32 *)SDL_malloc((size_t)W * H * 4);
  if (!pixels)
    return 0;
  for (int y = 0; y < H; y++)
    for (int x = 0; x < W; x++)
      pixels[y * W + x] = ((Uint32)(x * 4) << 16) | ((Uint32)(y * 4) << 8) |
                          (Uint32)(frame & 0xff) | 0xff000000u;

  SDL_Texture *tex =
      SDL_CreateTexture(r, SDL_PIXELFORMAT_ARGB8888,
                        SDL_TEXTUREACCESS_STREAMING, W, H);
  if (!tex) {
    SDL_free(pixels);
    return 0;
  }
  SDL_UpdateTexture(tex, NULL, pixels, W * 4);
  SDL_free(pixels);

  SDL_Rect dst = {(frame * 2) % (DEMO_W - W), 40, W, H};
  SDL_RenderCopy(r, tex, NULL, &dst);
  SDL_DestroyTexture(tex);
  return 1;
}

static Uint32 checksum(SDL_Renderer *r, Uint32 *out) {
  Uint32 *px = (Uint32 *)SDL_malloc((size_t)DEMO_W * DEMO_H * 4);
  if (!px)
    return 0;
  if (SDL_RenderReadPixels(r, NULL, SDL_PIXELFORMAT_ARGB8888, px,
                           DEMO_W * 4) != 0) {
    SDL_free(px);
    return 0;
  }
  Uint32 sum = 0;
  for (int i = 0; i < DEMO_W * DEMO_H; i++)
    sum = sum * 31u + (px[i] & 0xffffffu);
  *out = sum;
  SDL_free(px);
  return 1;
}

//--- event path -------------------------------------------------------------//

// Push an event, poll it back and check the payload survived both directions
// through a 56-byte union. The basic renderer has no input anyway, so this is
// the only event the demo can produce on its own.
static int eventRoundTrip(int *count) {
  SDL_Event ev;
  SDL_zero(ev);
  ev.type = SDL_USEREVENT;
  ev.user.code = 7;
  ev.user.data1 = (void *)(size_t)0x1234;
  if (SDL_PushEvent(&ev) < 0)
    return 0;

  *count = 0;
  while (SDL_PollEvent(&ev)) {
    if (ev.type == SDL_USEREVENT && ev.user.code == 7 &&
        ev.user.data1 == (void *)(size_t)0x1234)
      (*count)++;
  }
  return 1;
}

//--- dynamic loading --------------------------------------------------------//

// SDL_LoadObject hands back a raw native address which is then called through
// a function pointer: the shape CIL is least suited to, so prove it works.
static int probeDynamicLoad(void) {
  void *handle = SDL_LoadObject("SDL2.dll");
  if (!handle)
    return 0;
  int (*getCPUCount)(void) = (int (*)(void))SDL_LoadFunction(handle, "SDL_GetCPUCount");
  int cpus = getCPUCount ? getCPUCount() : -1;
  SDL_UnloadObject(handle);
  printf("dynamics cpus>0 %d\n", cpus > 0);
  return cpus > 0;
}

//--- GL ---------------------------------------------------------------------//

// Only a capability check: creating a 3.3 core context and looking up one
// entry point that opengl32.dll does not export proves the proc-loader path.
static int probeGL(void) {
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
  SDL_Window *w = SDL_CreateWindow("gl", 0, 0, 64, 64,
                                   SDL_WINDOW_HIDDEN | SDL_WINDOW_OPENGL);
  if (!w) {
    printf("gl context 0\n");
    return 0;
  }
  SDL_GLContext ctx = SDL_GL_CreateContext(w);
  if (!ctx) {
    SDL_DestroyWindow(w);
    printf("gl context 0\n");
    return 0;
  }
  void *createShader = SDL_GL_GetProcAddress("glCreateShader");
  printf("gl context 1 proc %d\n", createShader != NULL);
  SDL_GL_DeleteContext(ctx);
  SDL_DestroyWindow(w);
  return createShader != NULL;
}

//--- main -------------------------------------------------------------------//

int main(int argc, char **argv) {
  for (int i = 1; i < argc; i++) {
    if (!strcmp(argv[i], "--frames") && i + 1 < argc)
      gFrames = atoi(argv[++i]);
    else if (!strcmp(argv[i], "--visible"))
      gVisible = 1;
    else if (!strcmp(argv[i], "--gl"))
      gWantGL = 1;
  }

  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0)
    return fail("init");

  int rc = 0;
  SDL_Renderer *renderer = NULL;
  SDL_Window *window = SDL_CreateWindow(
      "llvm-msil SDL demo", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
      DEMO_W, DEMO_H, gVisible ? SDL_WINDOW_SHOWN : SDL_WINDOW_HIDDEN);
  if (!window) {
    rc = fail("window");
    goto done;
  }
  renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
  if (!renderer) {
    rc = fail("renderer");
    goto done;
  }

  if (gWantGL)
    probeGL();

  // Timer callbacks arrive on an SDL thread.
  SDL_TimerID timer = SDL_AddTimer(5, onTimer, NULL);

  // The demo's own frames: each one is drawn, optionally textured, presented
  // and checksummed. The last checksum is the frame the test compares.
  Uint32 sum = 0;
  for (int f = 0; f < gFrames; f++) {
    drawScene(renderer, f);
    if (f % 15 == 0)
      drawTextures(renderer, f);
    SDL_RenderPresent(renderer);
    if (!checksum(renderer, &sum)) {
      rc = fail("readpixels");
      goto done;
    }
    SDL_Event e;
    while (SDL_PollEvent(&e))
      if (e.type == SDL_QUIT) {
        f = gFrames;
        break;
      }
    SDL_Delay(1);
  }
  printf("frames %d checksum %08x\n", gFrames, sum);

  // Let the timer thread land at least one callback before removing it.
  for (int i = 0; i < 200 && gTimerTicks == 0; i++)
    SDL_Delay(5);
  if (timer)
    SDL_RemoveTimer(timer);
  printf("timer callbacks>0 %d\n", gTimerTicks > 0);

  int events = 0;
  if (!eventRoundTrip(&events)) {
    rc = fail("pushevent");
    goto done;
  }
  printf("events %d\n", events);

  if (!probeDynamicLoad()) {
    rc = fail("dynamicload");
    goto done;
  }

  printf("version %d.%d.%d\n", SDL_MAJOR_VERSION, SDL_MINOR_VERSION,
         SDL_PATCHLEVEL);
  printf("ok\n");

done:
  if (renderer)
    SDL_DestroyRenderer(renderer);
  if (window)
    SDL_DestroyWindow(window);
  SDL_Quit();
  return rc;
}
