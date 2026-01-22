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

#define WINDOW_W 1000
#define WINDOW_H WINDOW_W

#define GRID_W 800
#define GRID_H GRID_W
#define GRID_N 100
#define GRID_PIXELS_PER_CELL ((float)GRID_W / GRID_N)

#define VIS_STEPS_PER_ITER 100000

static uint8_t* data = NULL;
static int32_t data_len = 0;
static int32_t data_idx = 0;
static bool data_vis = false;

static Vector2 letter_pos[256] = {0};
static int8_t letter_set[256] = {0};
static int32_t letter_set_cnt = 0;

static Vector2 grid_pos = {0};
static Vector2 grid_center = {0};
static int32_t grid_counts[GRID_N][GRID_N] = {0};

static Vector2 point_pos = {0};
static float jump_ratio = 0.5f;

static void
cgr_init(void)
{
  grid_pos.x = (WINDOW_W - GRID_W) / 2;
  grid_pos.y = (WINDOW_H - GRID_H) / 2;

  grid_center.x = grid_pos.x + GRID_W / 2;
  grid_center.y = grid_pos.y + GRID_H / 2;

  point_pos = grid_center;

  for (int32_t i = 0; i < 256; i += 1) {
    letter_set[i] = 0;
  }
  for (int32_t i = 0; i < data_len; i += 1) {
    letter_set[data[i]] = 1;
  }
  letter_set_cnt = 0;
  for (int32_t i = 0; i < 256; i += 1) {
    if (letter_set[i] == 1) letter_set_cnt += 1;
  }

  for (int32_t i = 0, j = 0; i < 256; i += 1) {
    if (letter_set[i] == 0) continue;
    float s = 360.0f / letter_set_cnt;
    float r = GRID_W / 2;
    float a = (90.0f + s / 2 + j * s) * DEG2RAD;

    j += 1;
    letter_pos[i].x = grid_center.x + r * cosf(a);
    letter_pos[i].y = grid_center.y + r * sinf(a);
  }

  for (int32_t y = 0; y < GRID_N; y += 1) {
    for (int32_t x = 0; x < GRID_N; x += 1) {
      grid_counts[y][x] = 0;
    }
  }

  data_idx = 0;
  data_vis = false;
}

static void
cgr_draw_grid(void)
{
  float smax = 0.0f;
  for (int32_t y = 0; y < GRID_N; y += 1) {
    for (int32_t x = 0; x < GRID_N; x += 1) {
      float s = logf(1.0f + grid_counts[y][x]);
      if (s > smax) smax = s;
    }
  }

  for (int32_t y = 0; y < GRID_N; y += 1) {
    for (int32_t x = 0; x < GRID_N; x += 1) {
      float s = logf(1.0f + grid_counts[y][x]);
      Color c = BLACK;
      c.a = (uint8_t)(s / smax * 255.0f);

      Vector2 rp = {
        grid_pos.x + x * GRID_PIXELS_PER_CELL,
        grid_pos.y + y * GRID_PIXELS_PER_CELL,
      };
      Vector2 rs = {
        GRID_PIXELS_PER_CELL,
        GRID_PIXELS_PER_CELL,
      };
      DrawRectangleV(rp, rs, c);
    }
  }
}

static void
cgr_draw_letters(void)
{
  for (int32_t i = 0; i < 256; i += 1) {
    if (letter_set[i] == 0) continue;

    char buf[32] = {0};
    snprintf(buf, sizeof(buf), "%02x", i);
    Vector2 pos = Vector2Lerp(grid_center, letter_pos[i], 1.07);
    DrawText(buf, pos.x - 7.5f, pos.y - 15.0f, 10.0f, GRAY);
  }
}

// to generate a video out of images:
// ffmpeg -framerate 10 -pattern_type glob -i 'img-*.png' -c:v libx264 -pix_fmt yuv420p out.mp4
static void
cgr_export_screen(void)
{
  assert(ExportImage(LoadImageFromScreen(), "image.png"));
}

static void
cgr_vis_step(void)
{
  for (int32_t i = 0; i < VIS_STEPS_PER_ITER; i += 1) {
    if (!data_vis) return;
    if (data_idx >= data_len) return;
    if (letter_set[data[data_idx]] == 0) { data_idx += 1; continue; }

    Vector2 attr_pos = letter_pos[data[data_idx]];
    point_pos = Vector2Lerp(point_pos, attr_pos, jump_ratio);
    data_idx += 1;

    Vector2 p = Vector2Subtract(point_pos, grid_pos);
    int32_t i = (int32_t)(p.y / GRID_PIXELS_PER_CELL);
    int32_t j = (int32_t)(p.x / GRID_PIXELS_PER_CELL);
    grid_counts[i][j] += 1;
  }
}

static void
cgr_draw_debug_info(void)
{
  {
    char buf[32] = {0};
    snprintf(
      buf, sizeof(buf), "vis: %6.2f%%",
      (float)data_idx / data_len * 100.0f
    );
    DrawText(buf, 10.0f, 10.0f, 20.0f, GRAY);
  }
  {
    char buf[32] = {0};
    snprintf(buf, sizeof(buf), "ratio: %4.2f", jump_ratio);
    DrawText(buf, 10.0f, 30.0f, 20.0f, GRAY);
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

  cgr_read_sample(argv[1]);
  cgr_init();

  SetConfigFlags(FLAG_VSYNC_HINT);
  InitWindow(WINDOW_W, WINDOW_H, "CGR");

  while (!WindowShouldClose()) {
    if (IsKeyPressed(KEY_SPACE)) {
      data_vis = !data_vis;
    }
    if (IsKeyPressed(KEY_E)) {
      cgr_export_screen();
    }
    if (IsKeyPressed(KEY_R)) {
      if (jump_ratio < 1.0f) {
        jump_ratio += 0.1f;
        cgr_init();
      }
    }

    BeginDrawing();
    ClearBackground(RAYWHITE);

    cgr_vis_step();
    cgr_draw_grid();
    cgr_draw_letters();
    cgr_draw_debug_info();

    EndDrawing();
  }

  CloseWindow();

  return 0;
}
