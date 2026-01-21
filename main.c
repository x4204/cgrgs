#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "raylib.h"
#include "raymath.h"

#define SW 1000
#define SH SW

#define GW 800
#define GH GW
#define GN 100
#define GPPC ((float)GW / GN)

#define VIS_STEP_ITER 100000

static int8_t vb[256] = {0};
static int32_t vb_cnt = 0;
static Vector2 bp[256] = {0};
static Vector2 gp = {0};
static Vector2 gc = {0};
static int32_t gg[GN][GN] = {0};
static Vector2 pc = {0};
static uint8_t* data = NULL;
static int32_t data_len = 0;
static int32_t data_idx = 0;
static bool data_vis = false;
static float lerp_factor = 0.5f;
static bool go_next = false;
static bool reset_on_next = false;

static void
cgr_init(void)
{
  // vb['a'] = 1;
  // vb['c'] = 1;
  // vb['g'] = 1;
  // vb['t'] = 1;
  for (int32_t i = 0; i < 256; i += 1) {
    // if (isprint(i) || isspace(i)) vb[i] = 1;
    vb[i] = 1;
  }

  gp.x = (SW - GW) / 2;
  gp.y = (SH - GH) / 2;

  gc.x = gp.x + GW / 2;
  gc.y = gp.y + GH / 2;

  pc = gc;

  vb_cnt = 0;
  for (int32_t i = 0; i < 256; i += 1) {
    if (vb[i] == 1) vb_cnt += 1;
  }

  for (int32_t i = 0, j = 0; i < 256; i += 1) {
    if (vb[i] == 0) continue;
    float s = 360.0f / vb_cnt;
    float r = GW / 2;
    float a = (90.0f + s / 2 + j * s) * DEG2RAD;

    j += 1;
    bp[i].x = gc.x + r * cosf(a);
    bp[i].y = gc.y + r * sinf(a);
  }

  for (int32_t y = 0; y < GN; y += 1) {
    for (int32_t x = 0; x < GN; x += 1) {
      gg[y][x] = 0;
    }
  }

  data_vis = false;
  data_idx = 0;
}

static void
cgr_draw_lerp_factor(void)
{
  char buf[20] = {0};
  snprintf(buf, sizeof(buf), "LF=%.2f", lerp_factor);
  DrawText(buf, 10, 10, 30, GRAY);
}

static void
cgr_draw_grid(void)
{
  float smax = 0.0f;
  for (int32_t y = 0; y < GN; y += 1) {
    for (int32_t x = 0; x < GN; x += 1) {
      float s = logf(1.0f + gg[y][x]);
      if (s > smax) smax = s;
    }
  }

  for (int32_t y = 0; y < GN; y += 1) {
    for (int32_t x = 0; x < GN; x += 1) {
      float s = logf(1.0f + gg[y][x]);
      Color c = BLACK;
      c.a = (uint8_t)(s / smax * 255.0f);

      Vector2 rp = {gp.x + x * GPPC, gp.y + y * GPPC};
      Vector2 rs = {GPPC, GPPC};
      DrawRectangleV(rp, rs, c);
    }
  }
}

static void
cgr_draw_letters(void)
{
  for (int32_t i = 0; i < 256; i += 1) {
    if (vb[i] == 0) continue;

    char buf[20] = {0};
    snprintf(buf, sizeof(buf), "%02x", i);
    Vector2 pos = Vector2Lerp(gc, bp[i], 1.07);
    DrawText(buf, pos.x - 7.5f, pos.y - 15.0f, 10.0f, GRAY);
  }
}

// to generate a video out of images:
// ffmpeg -framerate 10 -pattern_type glob -i 'img-*.png' -c:v libx264 -pix_fmt yuv420p out.mp4
static void
cgr_export_screen(void)
{
  static int32_t n = 0;
  char path[30] = {0};
  snprintf(path, sizeof(path), "img-%03d.png", n);
  assert(ExportImage(LoadImageFromScreen(), path));
  n += 1;
}

static void
cgr_vis_step(void)
{
  for (int32_t i = 0; i < VIS_STEP_ITER; i += 1) {
    if (!data_vis) return;
    if (lerp_factor > 1.0f) return;
    if (data_idx >= data_len) { go_next = true; return; }
    if (vb[data[data_idx]] == 0) { data_idx += 1; continue; }

    pc = Vector2Lerp(pc, bp[data[data_idx]], lerp_factor);
    data_idx += 1;

    Vector2 p = Vector2Subtract(pc, gp);
    int32_t i = (int32_t)(p.y / GPPC);
    int32_t j = (int32_t)(p.x / GPPC);
    gg[i][j] += 1;
  }
}

static void
cgr_read_sample(char* path)
{
  struct stat st = {0};
  if (stat(path, &st) < 0) {
    printf("ERROR: cgr_read_sample: %s\n", strerror(errno));
    exit(1);
  }

  if (st.st_size == 0) {
    printf("ERROR: cgr_read_sample: file is empty\n");
    exit(1);
  }

  if (st.st_size > INT32_MAX) {
    printf("ERROR: cgr_read_sample: file too big (%ld bytes)", st.st_size);
    exit(1);
  }

  int fd = open(path, O_RDONLY);
  if (fd < 0) {
    printf("ERROR: cgr_read_sample: %s\n", strerror(errno));
    exit(1);
  }

  data_len = (int32_t)st.st_size;
  data = malloc(sizeof(*data) * data_len);
  ssize_t read_len = read(fd, data, data_len);
  if (read_len < 0) {
    printf("ERROR: cgr_read_sample: %s\n", strerror(errno));
    exit(1);
  }
  assert(read_len != 0);

  close(fd);
}

int
main(int argc, char** argv)
{
  if (argc != 2) {
    printf("ERROR: <file> not provided\n");
    printf("USAGE: %s <file>\n", argv[0]);
    exit(1);
  }

  cgr_init();
  cgr_read_sample(argv[1]);

  SetConfigFlags(FLAG_VSYNC_HINT);
  InitWindow(SW, SH, "CGR");

  while (!WindowShouldClose()) {
    if (IsKeyPressed(KEY_SPACE)) {
      data_vis = !data_vis;
    }

    BeginDrawing();
    ClearBackground(RAYWHITE);

    cgr_vis_step();
    // cgr_draw_lerp_factor();
    cgr_draw_grid();
    cgr_draw_letters();

    EndDrawing();

    if (reset_on_next && go_next) {
      cgr_export_screen();
      cgr_init();
      lerp_factor += 0.01f;
      data_vis = true;
      go_next = false;
    }
  }

  CloseWindow();

  return 0;
}
