//===-- sdl3_port.c - SDL3's own source, built by llvm-msil ----------------===//
//
// Not a program that *uses* SDL: SDL itself is compiled to CIL and linked in,
// so every SDL_* call below resolves to a method emitted from SDL's C. This
// is the port test.
//
// Prints one line per stage so a failure says how far it got, and exits 0 only
// when the whole chain worked.
//===----------------------------------------------------------------------===//

#include <SDL3/SDL.h>
#include <stdio.h>

int main(void) {
  if (!SDL_Init(SDL_INIT_VIDEO)) {
    printf("init 0 %s\n", SDL_GetError());
    return 1;
  }
  printf("init 1\n");
  printf("driver %s\n", SDL_GetCurrentVideoDriver());

  SDL_Window *window = SDL_CreateWindow("port", 320, 200, SDL_WINDOW_HIDDEN);
  printf("window %d\n", window != NULL);
  if (window) {
    printf("size %d %d\n", (int)SDL_GetWindowSurface(window)->w, 0);
    SDL_DestroyWindow(window);
  }

  SDL_Quit();
  printf("quit 1\n");
  return 0;
}
