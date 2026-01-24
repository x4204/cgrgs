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
#define GRID_N 400
#define GRID_PIXELS_PER_CELL ((float)GRID_W / GRID_N)

#define VIS_STEPS_PER_ITER 100000

#define N_CORNERS 4

static Vector2 corner_pos[N_CORNERS] = {0};
static uint8_t corner_map[256] = {
  [ 65] = 0, [ 67] = 1, [ 71] = 2, [ 84] = 3,
  [ 97] = 0, [ 99] = 1, [103] = 2, [116] = 3,
  [  0] = 0, [  1] = 1, [  2] = 2, [  3] = 3,
  [  4] = 0, [  5] = 1, [  6] = 2, [  7] = 3,
  [  8] = 0, [  9] = 1, [ 10] = 2, [ 11] = 3,
  [ 12] = 0, [ 13] = 1, [ 14] = 2, [ 15] = 3,
  [ 16] = 0, [ 17] = 1, [ 18] = 2, [ 19] = 3,
  [ 20] = 0, [ 21] = 1, [ 22] = 2, [ 23] = 3,
  [ 24] = 0, [ 25] = 1, [ 26] = 2, [ 27] = 3,
  [ 28] = 0, [ 29] = 1, [ 30] = 2, [ 31] = 3,
  [ 32] = 0, [ 33] = 1, [ 34] = 2, [ 35] = 3,
  [ 36] = 0, [ 37] = 1, [ 38] = 2, [ 39] = 3,
  [ 40] = 0, [ 41] = 1, [ 42] = 2, [ 43] = 3,
  [ 44] = 0, [ 45] = 1, [ 46] = 2, [ 47] = 3,
  [ 48] = 0, [ 49] = 1, [ 50] = 2, [ 51] = 3,
  [ 52] = 0, [ 53] = 1, [ 54] = 2, [ 55] = 3,
  [ 56] = 0, [ 57] = 1, [ 58] = 2, [ 59] = 3,
  [ 60] = 0, [ 61] = 1, [ 62] = 2, [ 63] = 3,
  [ 64] = 0, [ 66] = 1, [ 68] = 2, [ 69] = 3,
  [ 70] = 0, [ 72] = 1, [ 73] = 2, [ 74] = 3,
  [ 75] = 0, [ 76] = 1, [ 77] = 2, [ 78] = 3,
  [ 79] = 0, [ 80] = 1, [ 81] = 2, [ 82] = 3,
  [ 83] = 0, [ 85] = 1, [ 86] = 2, [ 87] = 3,
  [ 88] = 0, [ 89] = 1, [ 90] = 2, [ 91] = 3,
  [ 92] = 0, [ 93] = 1, [ 94] = 2, [ 95] = 3,
  [ 96] = 0, [ 98] = 1, [100] = 2, [101] = 3,
  [102] = 0, [104] = 1, [105] = 2, [106] = 3,
  [107] = 0, [108] = 1, [109] = 2, [110] = 3,
  [111] = 0, [112] = 1, [113] = 2, [114] = 3,
  [115] = 0, [117] = 1, [118] = 2, [119] = 3,
  [120] = 0, [121] = 1, [122] = 2, [123] = 3,
  [124] = 0, [125] = 1, [126] = 2, [127] = 3,
  [128] = 0, [129] = 1, [130] = 2, [131] = 3,
  [132] = 0, [133] = 1, [134] = 2, [135] = 3,
  [136] = 0, [137] = 1, [138] = 2, [139] = 3,
  [140] = 0, [141] = 1, [142] = 2, [143] = 3,
  [144] = 0, [145] = 1, [146] = 2, [147] = 3,
  [148] = 0, [149] = 1, [150] = 2, [151] = 3,
  [152] = 0, [153] = 1, [154] = 2, [155] = 3,
  [156] = 0, [157] = 1, [158] = 2, [159] = 3,
  [160] = 0, [161] = 1, [162] = 2, [163] = 3,
  [164] = 0, [165] = 1, [166] = 2, [167] = 3,
  [168] = 0, [169] = 1, [170] = 2, [171] = 3,
  [172] = 0, [173] = 1, [174] = 2, [175] = 3,
  [176] = 0, [177] = 1, [178] = 2, [179] = 3,
  [180] = 0, [181] = 1, [182] = 2, [183] = 3,
  [184] = 0, [185] = 1, [186] = 2, [187] = 3,
  [188] = 0, [189] = 1, [190] = 2, [191] = 3,
  [192] = 0, [193] = 1, [194] = 2, [195] = 3,
  [196] = 0, [197] = 1, [198] = 2, [199] = 3,
  [200] = 0, [201] = 1, [202] = 2, [203] = 3,
  [204] = 0, [205] = 1, [206] = 2, [207] = 3,
  [208] = 0, [209] = 1, [210] = 2, [211] = 3,
  [212] = 0, [213] = 1, [214] = 2, [215] = 3,
  [216] = 0, [217] = 1, [218] = 2, [219] = 3,
  [220] = 0, [221] = 1, [222] = 2, [223] = 3,
  [224] = 0, [225] = 1, [226] = 2, [227] = 3,
  [228] = 0, [229] = 1, [230] = 2, [231] = 3,
  [232] = 0, [233] = 1, [234] = 2, [235] = 3,
  [236] = 0, [237] = 1, [238] = 2, [239] = 3,
  [240] = 0, [241] = 1, [242] = 2, [243] = 3,
  [244] = 0, [245] = 1, [246] = 2, [247] = 3,
  [248] = 0, [249] = 1, [250] = 2, [251] = 3,
  [252] = 0, [253] = 1, [254] = 2, [255] = 3,
};

static uint8_t* data = NULL;
static int32_t data_len = 0;
static int32_t data_idx = 0;
static bool data_vis = false;

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

  for (int32_t i = 0; i < N_CORNERS; i += 1) {
    corner_pos[1].x = grid_pos.x;
    corner_pos[1].y = grid_pos.y;

    corner_pos[0].x = corner_pos[1].x;
    corner_pos[0].y = corner_pos[1].y + GRID_H;

    corner_pos[2].x = corner_pos[1].x + GRID_W;
    corner_pos[2].y = corner_pos[1].y;

    corner_pos[3].x = corner_pos[1].x + GRID_W;
    corner_pos[3].y = corner_pos[1].y + GRID_H;
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
cgr_draw_corners(void)
{
  for (int32_t i = 0; i < N_CORNERS; i += 1) {
    char buf[32] = {0};
    snprintf(buf, sizeof(buf), "%d", i);
    Vector2 pos = Vector2Lerp(grid_center, corner_pos[i], 1.07);
    DrawText(buf, pos.x - 7.5f, pos.y - 15.0f, 20.0f, GRAY);
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

    Vector2 attr_pos = corner_pos[corner_map[data[data_idx]]];
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
    cgr_draw_corners();
    cgr_draw_debug_info();

    EndDrawing();
  }

  CloseWindow();

  return 0;
}
