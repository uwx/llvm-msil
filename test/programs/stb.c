// End-to-end exercise of the stb single-header libraries: image decode and
// encode round trip, trueType rasterisation and metrics, rectangle packing,
// and the stb_ds container macros. Output is compared against a native build
// by stb.sh, so every result that reaches stdout has to be deterministic.

#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_SIMD
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"   // carries its own stb_rect_pack in STBRP_STATIC mode
#define STB_DS_IMPLEMENTATION
#include "stb_ds.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned long hash_bytes(const unsigned char *p, int n) {
  unsigned long h = 0;
  for (int i = 0; i < n; i++)
    h = h * 31u + p[i];
  return h;
}

static unsigned char *g_png;
static int g_png_len;

static void on_write(void *ctx, void *data, int size) {
  (void)ctx;
  for (int i = 0; i < size; i++)
    g_png[g_png_len++] = ((unsigned char *)data)[i];
}

// stb_image decode, a nearest-neighbour resize, and stb_image_write encode.
static int image(const char *dir) {
  char path[1024];
  snprintf(path, sizeof path, "%s/map_01.png", dir);

  int w, h, n, req = 4;
  unsigned char *px = stbi_load(path, &w, &h, &n, req);
  if (!px) {
    printf("image: load failed\n");
    return 3;
  }
  printf("image %dx%d ch=%d %d\n", w, h, n, stbi_is_16_bit(path));
  printf("image sum=%lu\n", hash_bytes(px, w * h * 4));

  int w2 = w / 2, h2 = h / 2;
  unsigned char *small = malloc((size_t)w2 * h2 * 4);
  for (int y = 0; y < h2; y++)
    for (int x = 0; x < w2; x++)
      for (int c = 0; c < 4; c++)
        small[(y * w2 + x) * 4 + c] = px[((y * 2) * w + x * 2) * 4 + c];

  g_png = malloc(4 * 1024 * 1024);
  if (!stbi_write_png_to_func(on_write, 0, w2, h2, 4, small, w2 * 4)) {
    printf("image: encode failed\n");
    return 4;
  }
  printf("image png=%d\n", g_png_len);

  // Decode our own output and check it round-trips.
  int w3, h3, n3;
  unsigned char *back = stbi_load_from_memory(g_png, g_png_len, &w3, &h3, &n3, req);
  unsigned long want = hash_bytes(small, w2 * h2 * 4);
  unsigned long got = back ? hash_bytes(back, w3 * h3 * 4) : 0;
  printf("image roundtrip %d %d %lu\n",
         w3 == w2 && h3 == h2 && got == want, w3, got);
  free(back);
  free(g_png);
  free(small);
  free(px);
  return 0;
}

static int truetype(const char *font) {
  static unsigned char ttf[16 * 1024 * 1024];
  FILE *f = fopen(font, "rb");
  if (!f) {
    printf("text: no font\n");
    return 3;
  }
  size_t len = fread(ttf, 1, sizeof ttf, f);
  fclose(f);
  printf("text bytes=%u\n", (unsigned)len);

  stbtt_fontinfo fi;
  if (!stbtt_InitFont(&fi, ttf, stbtt_GetFontOffsetForIndex(ttf, 0))) {
    printf("text: init failed\n");
    return 4;
  }
  int ascent, descent, linegap;
  stbtt_GetFontVMetrics(&fi, &ascent, &descent, &linegap);
  printf("text vmetrics %d %d %d glyphs=%d\n", ascent, descent, linegap,
         fi.numGlyphs);

  // Rasterise every printable ASCII glyph at a few sizes.
  unsigned long glyphsum = 0;
  int px = 0;
  for (int size = 8; size <= 24; size += 8) {
    float scale = stbtt_ScaleForPixelHeight(&fi, (float)size);
    for (int c = 32; c < 127; c++) {
      int x0, y0, x1, y1;
      stbtt_GetCodepointBitmapBox(&fi, c, scale, scale, &x0, &y0, &x1, &y1);
      int w = x1 - x0, h = y1 - y0;
      if (w <= 0 || h <= 0)
        continue;
      unsigned char *bm = malloc((size_t)w * h);
      stbtt_MakeCodepointBitmap(&fi, bm, w, h, w, scale, scale, c);
      glyphsum = glyphsum * 31u + hash_bytes(bm, w * h);
      px += w * h;
      free(bm);
    }
  }
  printf("text glyphs=%lu px=%d\n", glyphsum, px);

  // Advance along a string, applying kerning: the metrics paths.
  float scale = stbtt_ScaleForPixelHeight(&fi, 16.0f);
  const char *msg = "The quick brown fox jumps over the lazy dog 0123456789";
  float xpos = 0;
  for (const char *p = msg; *p; p++) {
    int adv, lsb;
    stbtt_GetCodepointHMetrics(&fi, *p, &adv, &lsb);
    xpos += adv * scale;
    if (p[1])
      xpos += stbtt_GetCodepointKernAdvance(&fi, *p, p[1]) * scale;
  }
  printf("text width=%.3f\n", xpos);
  return 0;
}

static void rectpack(void) {
  enum { NRECT = 128 };
  stbrp_rect rects[NRECT];
  for (int i = 0; i < NRECT; i++) {
    rects[i].id = i;
    rects[i].w = 4 + (i * 7) % 29;
    rects[i].h = 4 + (i * 11) % 23;
  }
  stbrp_context ctx;
  stbrp_node nodes[512];
  stbrp_init_target(&ctx, 256, 256, nodes, 512);
  stbrp_pack_rects(&ctx, rects, NRECT);
  unsigned long sum = 0;
  int packed = 0;
  for (int i = 0; i < NRECT; i++)
    if (rects[i].was_packed) {
      packed++;
      sum = sum * 31u + (unsigned)rects[i].x * 1009u + rects[i].y;
    }
  printf("rect packed=%d sum=%lu\n", packed, sum);
}

static void containers(void) {
  struct {
    char *key;
    int value;
  } *map = NULL;
  for (int i = 0; i < 200; i++) {
    char buf[32];
    snprintf(buf, sizeof buf, "key%d", i);
    shput(map, strdup(buf), i * 3);
  }
  int *arr = NULL;
  for (int i = 0; i < 500; i++)
    arrpush(arr, i * i);
  unsigned long sum = 0;
  for (int i = 0; i < hmlen(map); i++)
    sum = sum * 31u + map[i].value;
  for (int i = 0; i < arrlen(arr); i++)
    sum = sum * 31u + arr[i];
  printf("ds map=%d arr=%d sum=%lu get=%d\n", shlen(map), (int)arrlen(arr), sum,
         shget(map, "key7"));
}

int main(int argc, char **argv) {
  // argv[1] is the stb repository (map_01.png lives in data/), argv[2] the TTF.
  if (argc < 3) {
    printf("usage: %s <stb dir> <font>\n", argv[0]);
    return 2;
  }
  char data[1024];
  snprintf(data, sizeof data, "%s/data", argv[1]);

  int rc = image(data);
  if (rc)
    return rc;
  rc = truetype(argv[2]);
  if (rc)
    return rc;
  rectpack();
  containers();
  return 0;
}
