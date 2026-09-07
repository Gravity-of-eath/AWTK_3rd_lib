/**
 * conner_gradient 桌面动态 demo（x86）
 *
 * 三个控件同时动画：
 *   左  : cache=TRUE  （默认，直写像素 + 帧间缓存）
 *   中  : cache=FALSE （每帧走 canvas_fill_rect，用来肉眼比对是否一致）
 *   右  : 整圆圆环，验证 360 度不再退化成一条细线
 * 控制台每秒打印实测帧率。
 *
 * 用法: conner_gradient_demo [控件边长] [帧间隔ms]
 *   控件边长  默认 200，范围 40~2000。窗口大小按它自动算。
 *   帧间隔ms  默认 16(约60fps)。设成 1 可以让绘制成为瓶颈，用来观察真实上限。
 */
#include <stdio.h>
#include <time.h>
#include "awtk.h"
#include "conner_gradient_view.h"
#include "conner_gradient_view_register.h"

#define GAUGE_MIN 40
#define GAUGE_MAX 2000
#define MARGIN 12

static int32_t s_gauge = 200; /* 控件边长，可由命令行覆盖 */

static widget_t* s_g[3];
static int32_t s_value = 0;
static int32_t s_dir = 1;
static uint32_t s_frames = 0;
static double s_t0 = 0;

static double now_s(void) {
  struct timespec t;
  clock_gettime(CLOCK_MONOTONIC, &t);
  return t.tv_sec + t.tv_nsec / 1e9;
}

static ret_t on_tick(const timer_info_t* info) {
  int i;
  double t;

  s_value += s_dir * 2;
  if (s_value >= 100) { s_value = 100; s_dir = -1; }
  if (s_value <= 0) { s_value = 0; s_dir = 1; }

  for (i = 0; i < 3; i++) {
    widget_set_prop_int(s_g[i], "current", s_value);
  }

  s_frames++;
  t = now_s();
  if (t - s_t0 >= 1.0) {
    printf("current=%3d   %.1f 帧/秒\n", s_value, s_frames / (t - s_t0));
    fflush(stdout);
    s_frames = 0;
    s_t0 = t;
  }

  (void)info;
  return RET_REPEAT;
}

static widget_t* make_gauge(widget_t* win, int32_t x, const char* c0, const char* c1,
                            float start, float stop, float ratio, bool_t cache) {
  widget_t* g = conner_gradient_view_create(win, x, MARGIN, s_gauge, s_gauge);
  widget_set_prop_str(g, "start_color", c0);
  widget_set_prop_str(g, "stop_color", c1);
  widget_set_prop_float(g, "start_angle", start);
  widget_set_prop_float(g, "stop_angle", stop);
  widget_set_prop_float(g, "full_ratio", ratio);
  widget_set_prop_int(g, "max", 100);
  widget_set_prop_bool(g, "cache", cache);
  return g;
}

int main(int argc, char* argv[]) {
  widget_t* win = NULL;
  int32_t win_w = 0, win_h = 0, step = 0;
  uint32_t interval = 16;

  if (argc > 1) {
    if (tk_str_eq(argv[1], "-h") || tk_str_eq(argv[1], "--help")) {
      printf("用法: %s [控件边长] [帧间隔ms]\n", argv[0]);
      printf("  控件边长  默认 200，范围 %d~%d\n", GAUGE_MIN, GAUGE_MAX);
      printf("  帧间隔ms  默认 16(约60fps)，设成 1 可观察绘制的真实上限\n");
      return 0;
    }
    s_gauge = tk_atoi(argv[1]);
    if (s_gauge < GAUGE_MIN) s_gauge = GAUGE_MIN;
    if (s_gauge > GAUGE_MAX) s_gauge = GAUGE_MAX;
  }
  if (argc > 2) {
    interval = (uint32_t)tk_atoi(argv[2]);
    if (interval < 1) interval = 1;
  }

  step = s_gauge + MARGIN;
  win_w = MARGIN + step * 3;
  win_h = MARGIN * 2 + s_gauge;

  tk_init(win_w, win_h, APP_SIMULATOR, "conner_gradient_demo", NULL);
  conner_gradient_view_register();

  win = window_create(NULL, 0, 0, 0, 0);
  s_g[0] = make_gauge(win, MARGIN, "#00FF88", "#FF0066", 135, 405, 0.25f, TRUE);
  s_g[1] = make_gauge(win, MARGIN + step, "#00FF88", "#FF0066", 135, 405, 0.25f, FALSE);
  s_g[2] = make_gauge(win, MARGIN + step * 2, "#FF0000", "#0000FF", 0, 360, 0.30f, TRUE);

  printf("窗口 %dx%d  控件 %dx%d  帧间隔 %ums\n", win_w, win_h, s_gauge, s_gauge, interval);
  printf("左=cache开  中=cache关(应与左完全一致)  右=整圆\n");
  s_t0 = now_s();
  timer_add(on_tick, NULL, interval);

  return tk_run();
}
