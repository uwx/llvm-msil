// Runs a fixed number of DOOM tics and hashes the rendered framebuffer. The
// whole of DOOM is one 1.3 MB header, so this exercises the backend on a real
// program: floating point, fixed point, big structs and arrays, function
// pointers, varargs and recursion all at once. doom.sh compares the output
// against a native build.

#define DOOM_IMPLEMENTATION
#define DOOM_IMPLEMENT_PRINT
#define DOOM_IMPLEMENT_MALLOC
#define DOOM_IMPLEMENT_FILE_IO
#define DOOM_IMPLEMENT_EXIT
#define DOOM_IMPLEMENT_GETENV
#include "PureDOOM.h"

#include <stdio.h>

// One tic per query, so a run is reproducible rather than wall-clock paced.
static int vclock;

static void gettime_virtual(int *sec, int *usec) {
  *sec = vclock / 70;
  *usec = (vclock % 70) * 1000000 / 70;
  vclock++;
}

// The WAD directory is handed over through DOOMWADDIR rather than the working
// directory, so the caller controls which checkout is used.
static const char *wad_dir;

static char *getenv_doomwaddir(const char *name) {
  const char *want = "DOOMWADDIR";
  int i = 0;
  while (want[i] && name[i] == want[i])
    i++;
  return want[i] == 0 && name[i] == 0 ? (char *)wad_dir : 0;
}

static unsigned long hash_framebuffer(void) {
  const unsigned char *fb = doom_get_framebuffer(1);
  unsigned long h = 0;
  for (int i = 0; i < 320 * 200; i++)
    h = h * 31u + fb[i];
  return h;
}

int main(int argc, char **argv) {
  if (argc < 2) {
    printf("usage: %s <directory with doom1.wad>\n", argv[0]);
    return 2;
  }
  wad_dir = argv[1];

  doom_set_getenv(getenv_doomwaddir);
  doom_set_gettime(gettime_virtual);
  doom_init(1, argv, 0);

  // With the virtual clock, doom_update runs a tic whenever I_GetTime has
  // advanced; one call per tic keeps the sequence fixed.
  for (int i = 0; i < 175; i++)
    doom_update();
  printf("tics=%d\n", vclock);
  printf("framebuffer=%lu\n", hash_framebuffer());

  // Some movement, so input handling runs too.
  doom_key_down(DOOM_KEY_W);
  for (int i = 0; i < 35; i++)
    doom_update();
  doom_key_up(DOOM_KEY_W);
  printf("after-move=%lu\n", hash_framebuffer());

  return 0;
}
