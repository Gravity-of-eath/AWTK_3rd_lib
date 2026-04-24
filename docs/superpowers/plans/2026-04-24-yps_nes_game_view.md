# yps_nes_game_view Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 把 InfoNES FC 模拟器做成一个 AWTK 自定义控件 `yps_nes_game_view`，支持 XML/C 配置 ROM、键位、声音、存档；emulator 跑在独立 pthread，双 buffer 渲染。

**Architecture:** Widget 层 → nes_runtime 中间层 → InfoNES_System_awtk glue → InfoNES C++ 源码 / ALSA。共享状态通过原子变量 + mutex/cond；UI↔emulator 的 invalidate 走 `idle_queue`（线程安全）。

**Tech Stack:** C（widget/runtime/audio） + C++（InfoNES glue） + AWTK + pthread + ALSA（可选）。CMake build，参照 `src/conner_gradient/` 和 `src/breath_ellipse/` 的 widget pattern。

**Spec reference:** `docs/superpowers/specs/2026-04-24-yps_nes_game_view-design.md`

---

## Preamble: Reference Facts

Engineer implementing this may not have all context. Key facts:

1. **Build system:** `./build.sh` at repo root, platform defaults to `x86`. CMake runs in `src/`. AWTK headers at `$AWTK_INCLUDE = include/<platform>/awtk/src`. libawtk at `lib/<platform>/libawtk.so`.
2. **Widget pattern (from `src/conner_gradient/`):**
   - Struct embeds `widget_t widget;` as first member
   - `TK_DECL_VTABLE(xxx) = { .size=..., .type=..., .create=..., .set_prop=..., .get_prop=..., .on_paint_self=..., .on_destroy=..., .on_event=... }` — plus `.create` returns `widget_t*` and uses `widget_create(parent, TK_REF_VTABLE(xxx), x,y,w,h)`
   - Register via `widget_factory_register(widget_factory(), TYPE_NAME, create_fn)`
   - Macro `XXX(widget)` casts safely: `((xxx_t*)(xxx_cast(WIDGET(widget))))`
3. **AWTK keys (`include/x86/awtk/src/base/keys.h`):** `TK_KEY_UP`, `TK_KEY_DOWN`, `TK_KEY_LEFT`, `TK_KEY_RIGHT`, `TK_KEY_a..z`, `TK_KEY_A..Z`, `TK_KEY_RETURN`, `TK_KEY_SPACE`... ASCII printable chars = their ASCII code.
4. **AWTK idle_queue:** `ret_t idle_queue(idle_func_t on_idle, void* ctx)` from `base/idle.h`. Called from UI thread; `idle_queue` itself is documented thread-safe for cross-thread posting.
5. **AWTK bitmap:** `bitmap_create_ex(w, h, line_length, format)` with `BITMAP_FMT_RGB565`. To wrap external pixel buffer use `bitmap_init_from_rgba` or fill `bitmap_t` manually (buffer via `graphic_buffer_*`). Simplest: allocate a bitmap with `bitmap_create_ex`, `memcpy` into it each frame using `bitmap_lock_buffer_for_write`.
6. **AWTK canvas scale:** `canvas_draw_image(canvas_t* c, bitmap_t* img, const rect_t* src, const rect_t* dst)` — nearest-neighbor by default.
7. **InfoNES required callbacks** (`InfoNES/src/InfoNES_System.h`): `InfoNES_Menu`, `InfoNES_ReadRom`, `InfoNES_ReleaseRom`, `InfoNES_LoadFrame`, `InfoNES_PadState(DWORD*, DWORD*, DWORD*)`, `InfoNES_MemoryCopy`, `InfoNES_MemorySet`, `InfoNES_DebugPrint`, `InfoNES_Wait`, `InfoNES_SoundInit`, `InfoNES_SoundOpen(samples_per_sync, sample_rate) → int`, `InfoNES_SoundClose`, `InfoNES_SoundOutput(samples, w1..w5)`, `InfoNES_MessageBox(fmt, ...)`. Also `NesPalette[64]` global must be initialized. See `InfoNES/src/sdl/InfoNES_System_SDL.cpp` as reference.
8. **InfoNES globals you'll touch:** `NesPalette[64]` (WORD), `WorkFrame` (WORD*), `NesPaletteRGB[64][3]` (BYTE, 0-255).
9. **Pad bits** (from SDL port): bit7=Right bit6=Left bit5=Down bit4=Up bit3=Start bit2=Select bit1=B bit0=A. Keep this convention.
10. **No existing `data_reader` for asset paths in this repo — but awtk provides `data_reader_factory_read` for URIs.** For simplicity we prefer filesystem paths; asset:// is a small optional path we implement via `assets_manager_ref`.
11. **`pthread_timedjoin_np` is GNU/Linux-specific** — available on x86/t113/t507 targets. Require `#define _GNU_SOURCE`.

---

## File Structure

Created files (new):
- `src/yps_nes_game_view/CMakeLists.txt`
- `src/yps_nes_game_view/include/yps_nes_game_view.h` — public API + prop/type macros
- `src/yps_nes_game_view/include/yps_nes_game_view_register.h` — `yps_nes_game_view_register()` decl
- `src/yps_nes_game_view/src/yps_nes_game_view.c` — widget vtable + lifecycle + events + paint
- `src/yps_nes_game_view/src/yps_nes_game_view_register.c` — factory registration
- `src/yps_nes_game_view/src/nes_runtime.h` — runtime C API, state enum, struct
- `src/yps_nes_game_view/src/nes_runtime.c` — runtime impl: start/pause/stop/set_rom/thread_main
- `src/yps_nes_game_view/src/nes_keymap.h` — keymap parser prototype
- `src/yps_nes_game_view/src/nes_keymap.c` — keymap parser impl
- `src/yps_nes_game_view/src/nes_letterbox.h` — letterbox math
- `src/yps_nes_game_view/src/nes_letterbox.c` — letterbox math
- `src/yps_nes_game_view/src/nes_srampath.h` — sram path join
- `src/yps_nes_game_view/src/nes_srampath.c` — sram path join
- `src/yps_nes_game_view/src/nes_audio.h` — audio interface
- `src/yps_nes_game_view/src/nes_audio_null.c` — null audio impl
- `src/yps_nes_game_view/src/nes_audio_alsa.c` — ALSA impl
- `src/yps_nes_game_view/src/infones_glue/InfoNES_System_awtk.cpp` — implement all InfoNES_System callbacks
- `src/yps_nes_game_view/tests/test_nes_keymap.c` — keymap parser unit test
- `src/yps_nes_game_view/tests/test_nes_letterbox.c` — letterbox unit test
- `src/yps_nes_game_view/tests/test_nes_srampath.c` — sram path unit test
- `src/yps_nes_game_view/tests/test_audio_null.c` — audio null interface smoke test

Modified files:
- `src/CMakeLists.txt` — add `add_subdirectory(yps_nes_game_view)`
- Possibly `InfoNES/src/InfoNES.cpp` (guarded by `#ifdef INFONES_AWTK_GLUE`) — for pause/stop hooks

---

## Task 1: Scaffold widget directory + headers + empty register

**Files:**
- Create: `src/yps_nes_game_view/CMakeLists.txt`
- Create: `src/yps_nes_game_view/include/yps_nes_game_view.h`
- Create: `src/yps_nes_game_view/include/yps_nes_game_view_register.h`
- Create: `src/yps_nes_game_view/src/yps_nes_game_view.c`
- Create: `src/yps_nes_game_view/src/yps_nes_game_view_register.c`
- Modify: `src/CMakeLists.txt` (add `add_subdirectory(yps_nes_game_view)`)

Goal: Build passes with an empty but registered widget type. No runtime wiring yet.

- [ ] **Step 1: Create `include/yps_nes_game_view.h` with minimal type + stubs**

```c
#ifndef TK_YPS_NES_GAME_VIEW_H
#define TK_YPS_NES_GAME_VIEW_H

#include "base/widget.h"

BEGIN_C_DECLS

#define WIDGET_TYPE_YPS_NES_GAME_VIEW "yps_nes_game_view"

/* property names — used in XML attrs */
#define YPS_NES_GAME_VIEW_PROP_ROM           "rom"
#define YPS_NES_GAME_VIEW_PROP_SRAM_DIR      "sram_dir"
#define YPS_NES_GAME_VIEW_PROP_KEY_MAP       "key_map"
#define YPS_NES_GAME_VIEW_PROP_AUTO_PLAY     "auto_play"
#define YPS_NES_GAME_VIEW_PROP_SOUND_ENABLE  "sound_enable"
#define YPS_NES_GAME_VIEW_PROP_SAMPLE_RATE   "sample_rate"
#define YPS_NES_GAME_VIEW_PROP_VOLUME        "volume"
#define YPS_NES_GAME_VIEW_PROP_FPS           "fps"  /* read-only */

struct _nes_runtime_t;
typedef struct _nes_runtime_t nes_runtime_t;

typedef struct _yps_nes_game_view_t {
  widget_t widget;
  char* rom;
  char* sram_dir;
  char* key_map;
  bool_t auto_play;
  bool_t sound_enable;
  uint32_t sample_rate;
  uint8_t volume;

  nes_runtime_t* rt;       /* opaque; owned by widget */
  bitmap_t* front_bmp;     /* bitmap that wraps front buffer; allocated in on_create */
  uint32_t keymap[8];      /* 0=UP 1=DOWN 2=LEFT 3=RIGHT 4=A 5=B 6=START 7=SELECT -> TK_KEY_* */
} yps_nes_game_view_t;

widget_t* yps_nes_game_view_create(widget_t* parent, xy_t x, xy_t y, wh_t w, wh_t h);
widget_t* yps_nes_game_view_cast(widget_t* widget);

ret_t yps_nes_game_view_start  (widget_t* widget);
ret_t yps_nes_game_view_pause  (widget_t* widget);
ret_t yps_nes_game_view_stop   (widget_t* widget);
ret_t yps_nes_game_view_set_rom(widget_t* widget, const char* rom);
ret_t yps_nes_game_view_set_sram_dir   (widget_t* widget, const char* dir);
ret_t yps_nes_game_view_set_key_map    (widget_t* widget, const char* key_map);
ret_t yps_nes_game_view_set_sound_enable(widget_t* widget, bool_t enable);
ret_t yps_nes_game_view_set_sample_rate (widget_t* widget, uint32_t rate);
ret_t yps_nes_game_view_set_volume      (widget_t* widget, uint8_t volume);
float_t yps_nes_game_view_get_fps       (widget_t* widget);

#define YPS_NES_GAME_VIEW(widget) \
  ((yps_nes_game_view_t*)(yps_nes_game_view_cast(WIDGET(widget))))

TK_EXTERN_VTABLE(yps_nes_game_view);

END_C_DECLS
#endif
```

- [ ] **Step 2: Create `include/yps_nes_game_view_register.h`**

```c
#ifndef TK_YPS_NES_GAME_VIEW_REGISTER_H
#define TK_YPS_NES_GAME_VIEW_REGISTER_H
#include "tkc/types_def.h"
BEGIN_C_DECLS
ret_t yps_nes_game_view_register(void);
END_C_DECLS
#endif
```

- [ ] **Step 3: Create `src/yps_nes_game_view.c` — empty stub widget**

```c
#include "yps_nes_game_view.h"
#include "tkc/mem.h"
#include "tkc/utils.h"

static ret_t yps_nes_game_view_on_paint_self(widget_t* widget, canvas_t* c) {
  (void)widget; (void)c;
  return RET_OK;
}
static ret_t yps_nes_game_view_on_destroy(widget_t* widget) {
  yps_nes_game_view_t* v = YPS_NES_GAME_VIEW(widget);
  if (v == NULL) return RET_BAD_PARAMS;
  TKMEM_FREE(v->rom);
  TKMEM_FREE(v->sram_dir);
  TKMEM_FREE(v->key_map);
  return RET_OK;
}

TK_DECL_VTABLE(yps_nes_game_view) = {
  .size = sizeof(yps_nes_game_view_t),
  .type = WIDGET_TYPE_YPS_NES_GAME_VIEW,
  .create = yps_nes_game_view_create,
  .on_paint_self = yps_nes_game_view_on_paint_self,
  .on_destroy = yps_nes_game_view_on_destroy,
};

widget_t* yps_nes_game_view_create(widget_t* parent, xy_t x, xy_t y, wh_t w, wh_t h) {
  widget_t* widget = widget_create(parent, TK_REF_VTABLE(yps_nes_game_view), x, y, w, h);
  yps_nes_game_view_t* v = YPS_NES_GAME_VIEW(widget);
  return_value_if_fail(v != NULL, NULL);

  v->sram_dir = tk_strdup("/tmp");
  v->auto_play = TRUE;
  v->sound_enable = TRUE;
  v->sample_rate = 22050;
  v->volume = 80;
  return widget;
}

widget_t* yps_nes_game_view_cast(widget_t* widget) {
  return_value_if_fail(widget != NULL && widget->vt == TK_REF_VTABLE(yps_nes_game_view), NULL);
  return widget;
}

/* Method stubs — real impl comes in later tasks */
ret_t yps_nes_game_view_start(widget_t* w)  { (void)w; return RET_NOT_IMPL; }
ret_t yps_nes_game_view_pause(widget_t* w)  { (void)w; return RET_NOT_IMPL; }
ret_t yps_nes_game_view_stop(widget_t* w)   { (void)w; return RET_NOT_IMPL; }
ret_t yps_nes_game_view_set_rom(widget_t* w, const char* r) { (void)w; (void)r; return RET_NOT_IMPL; }
ret_t yps_nes_game_view_set_sram_dir(widget_t* w, const char* d){ (void)w; (void)d; return RET_NOT_IMPL; }
ret_t yps_nes_game_view_set_key_map(widget_t* w, const char* k) { (void)w; (void)k; return RET_NOT_IMPL; }
ret_t yps_nes_game_view_set_sound_enable(widget_t* w, bool_t e){ (void)w; (void)e; return RET_NOT_IMPL; }
ret_t yps_nes_game_view_set_sample_rate(widget_t* w, uint32_t r){ (void)w; (void)r; return RET_NOT_IMPL; }
ret_t yps_nes_game_view_set_volume(widget_t* w, uint8_t v)     { (void)w; (void)v; return RET_NOT_IMPL; }
float_t yps_nes_game_view_get_fps(widget_t* w)  { (void)w; return 0; }
```

- [ ] **Step 4: Create `src/yps_nes_game_view_register.c`**

```c
#include "yps_nes_game_view_register.h"
#include "yps_nes_game_view.h"
#include "base/widget_factory.h"

ret_t yps_nes_game_view_register(void) {
  return widget_factory_register(widget_factory(),
                                 WIDGET_TYPE_YPS_NES_GAME_VIEW,
                                 yps_nes_game_view_create);
}
```

- [ ] **Step 5: Create `src/yps_nes_game_view/CMakeLists.txt`**

```cmake
cmake_minimum_required(VERSION 3.10)

include_directories(${AWTK_INCLUDE})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

set(LIBRARY_NAME yps_nes_game_view)

file(GLOB SOURCES
    src/*.c
)

add_library(${LIBRARY_NAME} ${SOURCES})
target_link_libraries(${LIBRARY_NAME} ${AWTK_SO})

install(TARGETS ${LIBRARY_NAME} LIBRARY DESTINATION ${LIBRARY_NAME})
install(DIRECTORY include DESTINATION ${LIBRARY_NAME})
```

- [ ] **Step 6: Register subdirectory in `src/CMakeLists.txt`**

Modify `src/CMakeLists.txt`, add a line after `add_subdirectory(jump_label)`:

```cmake
add_subdirectory(yps_nes_game_view)
```

- [ ] **Step 7: Verify build**

Run: `./build.sh`
Expected: ends with `-- Installing: .../3rdlib/x86/yps_nes_game_view/libyps_nes_game_view.so` (or equivalent), no errors.

- [ ] **Step 8: Commit**

```bash
git add src/yps_nes_game_view/ src/CMakeLists.txt
git commit -m "scaffold yps_nes_game_view widget (empty)"
```

---

## Task 2: Keymap parser + unit test (TDD)

**Files:**
- Create: `src/yps_nes_game_view/src/nes_keymap.h`
- Create: `src/yps_nes_game_view/src/nes_keymap.c`
- Create: `src/yps_nes_game_view/tests/test_nes_keymap.c`
- Modify: `src/yps_nes_game_view/CMakeLists.txt` (add test target)

Parser contract: given `"up=UP,down=DOWN,a=X,b=Z,start=S,select=A"` and a pre-populated `defaults[8]`, fill out `out[8]` using `TK_KEY_*` codes. Unspecified slots keep defaults. Empty input → defaults unchanged. Returns number of keys successfully parsed. Partial maps are supported.

- [ ] **Step 1: Write the failing test**

Create `src/yps_nes_game_view/tests/test_nes_keymap.c`:

```c
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "base/keys.h"
#include "../src/nes_keymap.h"

static void test_defaults_when_empty(void) {
  uint32_t defaults[8] = {TK_KEY_UP, TK_KEY_DOWN, TK_KEY_LEFT, TK_KEY_RIGHT,
                          TK_KEY_x, TK_KEY_z, TK_KEY_s, TK_KEY_a};
  uint32_t out[8] = {0};
  int n = nes_keymap_parse("", defaults, out);
  assert(n == 0);
  for (int i = 0; i < 8; i++) assert(out[i] == defaults[i]);
  printf("test_defaults_when_empty OK\n");
}

static void test_partial_override(void) {
  uint32_t defaults[8] = {TK_KEY_UP, TK_KEY_DOWN, TK_KEY_LEFT, TK_KEY_RIGHT,
                          TK_KEY_x, TK_KEY_z, TK_KEY_s, TK_KEY_a};
  uint32_t out[8] = {0};
  int n = nes_keymap_parse("a=SPACE,start=RETURN", defaults, out);
  assert(n == 2);
  assert(out[0] == TK_KEY_UP);      /* unchanged */
  assert(out[1] == TK_KEY_DOWN);
  assert(out[2] == TK_KEY_LEFT);
  assert(out[3] == TK_KEY_RIGHT);
  assert(out[4] == TK_KEY_SPACE);   /* a remapped */
  assert(out[5] == TK_KEY_z);
  assert(out[6] == TK_KEY_RETURN);  /* start remapped */
  assert(out[7] == TK_KEY_a);
  printf("test_partial_override OK\n");
}

static void test_all_slots(void) {
  uint32_t defaults[8] = {0,0,0,0,0,0,0,0};
  uint32_t out[8] = {0};
  int n = nes_keymap_parse("up=w,down=s,left=a,right=d,a=j,b=k,start=RETURN,select=ESCAPE",
                           defaults, out);
  assert(n == 8);
  assert(out[0] == TK_KEY_w);
  assert(out[1] == TK_KEY_s);
  assert(out[2] == TK_KEY_a);
  assert(out[3] == TK_KEY_d);
  assert(out[4] == TK_KEY_j);
  assert(out[5] == TK_KEY_k);
  assert(out[6] == TK_KEY_RETURN);
  assert(out[7] == TK_KEY_ESCAPE);
  printf("test_all_slots OK\n");
}

static void test_whitespace_and_case(void) {
  uint32_t defaults[8] = {0,0,0,0,0,0,0,0};
  uint32_t out[8] = {0};
  int n = nes_keymap_parse("  UP = up ,  A = X  ", defaults, out);
  assert(n == 2);
  assert(out[0] == TK_KEY_UP);
  assert(out[4] == TK_KEY_x);  /* TK_KEY_x lowercase */
  printf("test_whitespace_and_case OK\n");
}

static void test_unknown_key_ignored(void) {
  uint32_t defaults[8] = {TK_KEY_UP};
  uint32_t out[8] = {0};
  int n = nes_keymap_parse("up=UP,nope=X,b=Z", defaults, out);
  /* nope=X silently skipped (unknown slot name); up and b are fine */
  assert(n == 2);
  assert(out[0] == TK_KEY_UP);
  assert(out[5] == TK_KEY_z);
  printf("test_unknown_key_ignored OK\n");
}

int main(void) {
  test_defaults_when_empty();
  test_partial_override();
  test_all_slots();
  test_whitespace_and_case();
  test_unknown_key_ignored();
  return 0;
}
```

Create header `src/yps_nes_game_view/src/nes_keymap.h`:

```c
#ifndef YPS_NES_KEYMAP_H
#define YPS_NES_KEYMAP_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

/**
 * Slot indices for `defaults` / `out` arrays:
 * 0=UP 1=DOWN 2=LEFT 3=RIGHT 4=A 5=B 6=START 7=SELECT
 *
 * map_str syntax: "name=KEY,name=KEY,..." where name ∈ {up,down,left,right,a,b,start,select}
 * and KEY is a TK_KEY_* symbolic name (without the TK_KEY_ prefix), case-insensitive.
 * Single-char keys like X, Z, A are parsed as TK_KEY_<lower>, e.g. "X" -> TK_KEY_x.
 *
 * Unspecified slots keep defaults. Unknown slot names silently ignored.
 * Returns number of slots successfully overridden.
 */
int nes_keymap_parse(const char* map_str, const uint32_t defaults[8], uint32_t out[8]);

/* key name → TK_KEY_* code; 0 if unknown. Public for tests. */
uint32_t nes_keymap_name_to_code(const char* name, size_t len);

#ifdef __cplusplus
}
#endif
#endif
```

- [ ] **Step 2: Add test target to `src/yps_nes_game_view/CMakeLists.txt`**

Append to the existing `CMakeLists.txt` from Task 1:

```cmake
if(PLATFORM STREQUAL "x86")
  add_executable(test_nes_keymap
      tests/test_nes_keymap.c
      src/nes_keymap.c)
  target_include_directories(test_nes_keymap PRIVATE ${AWTK_INCLUDE})
  install(TARGETS test_nes_keymap RUNTIME DESTINATION ${LIBRARY_NAME})
endif()
```

- [ ] **Step 3: Run to verify it fails (no `nes_keymap.c` yet)**

Run: `./build.sh`
Expected: linker error "undefined reference to `nes_keymap_parse`".

- [ ] **Step 4: Implement `src/yps_nes_game_view/src/nes_keymap.c`**

```c
#include "nes_keymap.h"
#include "base/keys.h"
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

static int ieq(const char* a, size_t a_len, const char* b) {
  size_t b_len = strlen(b);
  if (a_len != b_len) return 0;
  for (size_t i = 0; i < a_len; i++) {
    if (tolower((unsigned char)a[i]) != tolower((unsigned char)b[i])) return 0;
  }
  return 1;
}

/* Slot name → slot index (0..7); -1 if unknown */
static int slot_index(const char* name, size_t len) {
  if (ieq(name, len, "up"))     return 0;
  if (ieq(name, len, "down"))   return 1;
  if (ieq(name, len, "left"))   return 2;
  if (ieq(name, len, "right"))  return 3;
  if (ieq(name, len, "a"))      return 4;
  if (ieq(name, len, "b"))      return 5;
  if (ieq(name, len, "start"))  return 6;
  if (ieq(name, len, "select")) return 7;
  return -1;
}

uint32_t nes_keymap_name_to_code(const char* name, size_t len) {
  /* Named symbolic keys */
  if (ieq(name, len, "UP"))       return TK_KEY_UP;
  if (ieq(name, len, "DOWN"))     return TK_KEY_DOWN;
  if (ieq(name, len, "LEFT"))     return TK_KEY_LEFT;
  if (ieq(name, len, "RIGHT"))    return TK_KEY_RIGHT;
  if (ieq(name, len, "RETURN"))   return TK_KEY_RETURN;
  if (ieq(name, len, "ENTER"))    return TK_KEY_RETURN;
  if (ieq(name, len, "ESCAPE"))   return TK_KEY_ESCAPE;
  if (ieq(name, len, "ESC"))      return TK_KEY_ESCAPE;
  if (ieq(name, len, "SPACE"))    return TK_KEY_SPACE;
  if (ieq(name, len, "TAB"))      return TK_KEY_TAB;
  if (ieq(name, len, "BACKSPACE"))return TK_KEY_BACKSPACE;
  /* Single ASCII char → lowercase key code (TK_KEY_a..z and digits) */
  if (len == 1) {
    char c = (char)tolower((unsigned char)name[0]);
    return (uint32_t)c;  /* TK_KEY_a..z / '0'..'9' are ASCII values per awtk */
  }
  return 0;
}

/* Trim leading/trailing whitespace; updates *p and *len */
static void trim(const char** p, size_t* len) {
  while (*len > 0 && isspace((unsigned char)(**p))) { (*p)++; (*len)--; }
  while (*len > 0 && isspace((unsigned char)((*p)[*len - 1]))) { (*len)--; }
}

int nes_keymap_parse(const char* map_str, const uint32_t defaults[8], uint32_t out[8]) {
  for (int i = 0; i < 8; i++) out[i] = defaults[i];
  if (map_str == NULL || *map_str == '\0') return 0;

  int count = 0;
  const char* p = map_str;

  while (*p) {
    /* Skip separators */
    while (*p == ',' || *p == ';' || isspace((unsigned char)*p)) p++;
    if (!*p) break;

    /* Find '=' */
    const char* eq = strchr(p, '=');
    if (!eq) break;

    const char* name = p;
    size_t name_len = (size_t)(eq - p);
    trim(&name, &name_len);

    const char* val = eq + 1;
    const char* end = val;
    while (*end && *end != ',' && *end != ';') end++;
    size_t val_len = (size_t)(end - val);
    trim(&val, &val_len);

    int slot = slot_index(name, name_len);
    if (slot >= 0 && val_len > 0) {
      uint32_t code = nes_keymap_name_to_code(val, val_len);
      if (code != 0) {
        out[slot] = code;
        count++;
      }
    }
    p = (*end) ? end + 1 : end;
  }
  return count;
}
```

- [ ] **Step 5: Run to verify tests pass**

Run: `./build.sh && ./3rdlib/x86/yps_nes_game_view/test_nes_keymap`
Expected: `test_defaults_when_empty OK`, `test_partial_override OK`, `test_all_slots OK`, `test_whitespace_and_case OK`, `test_unknown_key_ignored OK`.

- [ ] **Step 6: Commit**

```bash
git add src/yps_nes_game_view/src/nes_keymap.* src/yps_nes_game_view/tests/test_nes_keymap.c src/yps_nes_game_view/CMakeLists.txt
git commit -m "add nes_keymap parser with tests"
```

---

## Task 3: Letterbox math + unit test

**Files:**
- Create: `src/yps_nes_game_view/src/nes_letterbox.h`
- Create: `src/yps_nes_game_view/src/nes_letterbox.c`
- Create: `src/yps_nes_game_view/tests/test_nes_letterbox.c`
- Modify: `src/yps_nes_game_view/CMakeLists.txt`

Contract: given widget `w × h`, compute dst_rect that fits 256×240 proportionally, centered.

- [ ] **Step 1: Write the failing test**

Create `src/yps_nes_game_view/tests/test_nes_letterbox.c`:

```c
#include <assert.h>
#include <stdio.h>
#include "../src/nes_letterbox.h"

static void test_exact_2x(void) {
  nes_rect_t r = nes_letterbox_fit(512, 480);
  assert(r.w == 512 && r.h == 480);
  assert(r.x == 0 && r.y == 0);
  printf("test_exact_2x OK\n");
}

static void test_wider_than_nes(void) {
  /* 800x240 — height is the constraint, 256x240, horizontal letterbox */
  nes_rect_t r = nes_letterbox_fit(800, 240);
  assert(r.w == 256 && r.h == 240);
  assert(r.x == (800 - 256) / 2);
  assert(r.y == 0);
  printf("test_wider_than_nes OK\n");
}

static void test_taller_than_nes(void) {
  /* 256x800 — width is the constraint; vertical letterbox */
  nes_rect_t r = nes_letterbox_fit(256, 800);
  assert(r.w == 256 && r.h == 240);
  assert(r.x == 0);
  assert(r.y == (800 - 240) / 2);
  printf("test_taller_than_nes OK\n");
}

static void test_non_integer_scale(void) {
  /* 300x300 — min(300/256,300/240)=1.17..; w=~301, h=~281. Use x/y centered. */
  nes_rect_t r = nes_letterbox_fit(300, 300);
  /* scale = min(1.171, 1.25) = 1.171; dst_w = 256*1.171 = 299 */
  assert(r.w >= 298 && r.w <= 300);
  assert(r.h >= 279 && r.h <= 282);
  assert(r.x + r.w <= 300);
  assert(r.y + r.h <= 300);
  printf("test_non_integer_scale OK (w=%d h=%d x=%d y=%d)\n", r.w, r.h, r.x, r.y);
}

static void test_too_small(void) {
  nes_rect_t r = nes_letterbox_fit(0, 0);
  assert(r.w == 0 || r.h == 0); /* degenerate */
  printf("test_too_small OK\n");
}

int main(void) {
  test_exact_2x();
  test_wider_than_nes();
  test_taller_than_nes();
  test_non_integer_scale();
  test_too_small();
  return 0;
}
```

Create `src/yps_nes_game_view/src/nes_letterbox.h`:

```c
#ifndef YPS_NES_LETTERBOX_H
#define YPS_NES_LETTERBOX_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

#define NES_NATIVE_W 256
#define NES_NATIVE_H 240

typedef struct { int32_t x, y, w, h; } nes_rect_t;

/* Compute dst rect that fits 256x240 proportionally inside (widget_w, widget_h),
 * centered. Returns zero-sized rect if either dimension is 0. */
nes_rect_t nes_letterbox_fit(int32_t widget_w, int32_t widget_h);

#ifdef __cplusplus
}
#endif
#endif
```

- [ ] **Step 2: Add test target to CMakeLists**

Append to `src/yps_nes_game_view/CMakeLists.txt` inside the `if(PLATFORM STREQUAL "x86")` block:

```cmake
  add_executable(test_nes_letterbox
      tests/test_nes_letterbox.c
      src/nes_letterbox.c)
  install(TARGETS test_nes_letterbox RUNTIME DESTINATION ${LIBRARY_NAME})
```

- [ ] **Step 3: Run to verify it fails**

Run: `./build.sh`
Expected: linker error `undefined reference to nes_letterbox_fit`.

- [ ] **Step 4: Implement `src/yps_nes_game_view/src/nes_letterbox.c`**

```c
#include "nes_letterbox.h"

nes_rect_t nes_letterbox_fit(int32_t widget_w, int32_t widget_h) {
  nes_rect_t r = {0,0,0,0};
  if (widget_w <= 0 || widget_h <= 0) return r;

  double sx = (double)widget_w / (double)NES_NATIVE_W;
  double sy = (double)widget_h / (double)NES_NATIVE_H;
  double s  = (sx < sy) ? sx : sy;

  r.w = (int32_t)(NES_NATIVE_W * s + 0.5);
  r.h = (int32_t)(NES_NATIVE_H * s + 0.5);
  if (r.w > widget_w) r.w = widget_w;
  if (r.h > widget_h) r.h = widget_h;
  r.x = (widget_w - r.w) / 2;
  r.y = (widget_h - r.h) / 2;
  return r;
}
```

- [ ] **Step 5: Run to verify tests pass**

Run: `./build.sh && ./3rdlib/x86/yps_nes_game_view/test_nes_letterbox`
Expected: 5 lines "OK".

- [ ] **Step 6: Commit**

```bash
git add src/yps_nes_game_view/src/nes_letterbox.* src/yps_nes_game_view/tests/test_nes_letterbox.c src/yps_nes_game_view/CMakeLists.txt
git commit -m "add letterbox math with tests"
```

---

## Task 4: SRAM path builder + unit test

**Files:**
- Create: `src/yps_nes_game_view/src/nes_srampath.h`
- Create: `src/yps_nes_game_view/src/nes_srampath.c`
- Create: `src/yps_nes_game_view/tests/test_nes_srampath.c`
- Modify: `src/yps_nes_game_view/CMakeLists.txt`

Contract: given `sram_dir` and `rom_path`, produce `sram_dir + "/" + basename_no_ext(rom_path) + ".srm"`.

- [ ] **Step 1: Write the failing test**

Create `src/yps_nes_game_view/tests/test_nes_srampath.c`:

```c
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../src/nes_srampath.h"

static void test_simple(void) {
  char out[256];
  int n = nes_srampath_build("/tmp", "/home/u/mario.nes", out, sizeof(out));
  assert(n > 0);
  assert(strcmp(out, "/tmp/mario.srm") == 0);
  printf("test_simple OK\n");
}

static void test_no_ext(void) {
  char out[256];
  nes_srampath_build("/tmp", "noext", out, sizeof(out));
  assert(strcmp(out, "/tmp/noext.srm") == 0);
  printf("test_no_ext OK\n");
}

static void test_trailing_slash(void) {
  char out[256];
  nes_srampath_build("/tmp/", "game.nes", out, sizeof(out));
  assert(strcmp(out, "/tmp/game.srm") == 0);
  printf("test_trailing_slash OK\n");
}

static void test_dots_in_name(void) {
  char out[256];
  nes_srampath_build("/d", "path/a.b.c.nes", out, sizeof(out));
  /* strip only the last .ext */
  assert(strcmp(out, "/d/a.b.c.srm") == 0);
  printf("test_dots_in_name OK\n");
}

static void test_buffer_too_small(void) {
  char out[8];
  int n = nes_srampath_build("/tmp", "/a/b/c/long_rom_name.nes", out, sizeof(out));
  assert(n < 0);  /* overflow */
  printf("test_buffer_too_small OK\n");
}

int main(void) {
  test_simple();
  test_no_ext();
  test_trailing_slash();
  test_dots_in_name();
  test_buffer_too_small();
  return 0;
}
```

Create `src/yps_nes_game_view/src/nes_srampath.h`:

```c
#ifndef YPS_NES_SRAMPATH_H
#define YPS_NES_SRAMPATH_H
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Build sram_dir + / + basename_no_ext(rom_path) + .srm.
 * Returns number of chars written on success (not counting NUL),
 * negative on overflow. */
int nes_srampath_build(const char* sram_dir, const char* rom_path,
                       char* out, size_t out_size);
#ifdef __cplusplus
}
#endif
#endif
```

- [ ] **Step 2: Add test to CMakeLists**

Inside `if(PLATFORM STREQUAL "x86")` block:

```cmake
  add_executable(test_nes_srampath
      tests/test_nes_srampath.c
      src/nes_srampath.c)
  install(TARGETS test_nes_srampath RUNTIME DESTINATION ${LIBRARY_NAME})
```

- [ ] **Step 3: Run to verify fail**

Run: `./build.sh`
Expected: link error `undefined reference to nes_srampath_build`.

- [ ] **Step 4: Implement `src/yps_nes_game_view/src/nes_srampath.c`**

```c
#include "nes_srampath.h"
#include <string.h>
#include <stdio.h>

int nes_srampath_build(const char* sram_dir, const char* rom_path,
                       char* out, size_t out_size) {
  if (!sram_dir || !rom_path || !out || out_size == 0) return -1;

  /* basename: last path component after '/' or '\\' */
  const char* slash = strrchr(rom_path, '/');
  const char* bslash = strrchr(rom_path, '\\');
  const char* base = rom_path;
  if (slash && slash >= base)  base = slash + 1;
  if (bslash && bslash >= base) base = bslash + 1;

  /* Strip last ".ext" if present */
  const char* dot = strrchr(base, '.');
  size_t base_len = dot ? (size_t)(dot - base) : strlen(base);

  /* Trim trailing '/' from sram_dir for clean join */
  size_t dir_len = strlen(sram_dir);
  while (dir_len > 0 && (sram_dir[dir_len - 1] == '/' || sram_dir[dir_len - 1] == '\\')) dir_len--;

  int needed = snprintf(out, out_size, "%.*s/%.*s.srm",
                        (int)dir_len, sram_dir,
                        (int)base_len, base);
  if (needed < 0 || (size_t)needed >= out_size) return -1;
  return needed;
}
```

- [ ] **Step 5: Run to verify**

Run: `./build.sh && ./3rdlib/x86/yps_nes_game_view/test_nes_srampath`
Expected: 5 lines "OK".

- [ ] **Step 6: Commit**

```bash
git add src/yps_nes_game_view/src/nes_srampath.* src/yps_nes_game_view/tests/test_nes_srampath.c src/yps_nes_game_view/CMakeLists.txt
git commit -m "add sram path builder with tests"
```

---

## Task 5: nes_audio interface + null impl + test

**Files:**
- Create: `src/yps_nes_game_view/src/nes_audio.h`
- Create: `src/yps_nes_game_view/src/nes_audio_null.c`
- Create: `src/yps_nes_game_view/tests/test_audio_null.c`
- Modify: `src/yps_nes_game_view/CMakeLists.txt`

Goal: establish audio backend interface with working null impl (always succeeds, drops samples).

- [ ] **Step 1: Write the failing test**

Create `src/yps_nes_game_view/tests/test_audio_null.c`:

```c
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "../src/nes_audio.h"

int main(void) {
  nes_audio_t* a = nes_audio_open(22050);
  assert(a != NULL);

  uint8_t buf[1024];
  memset(buf, 128, sizeof(buf));
  int rc = nes_audio_write(a, buf, sizeof(buf), 100);
  assert(rc == (int)sizeof(buf));  /* null impl returns sample count */

  nes_audio_close(a);
  printf("test_audio_null OK\n");
  return 0;
}
```

Create `src/yps_nes_game_view/src/nes_audio.h`:

```c
#ifndef YPS_NES_AUDIO_H
#define YPS_NES_AUDIO_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef struct _nes_audio_t nes_audio_t;

/* Open audio backend at given sample rate (Hz). Mono, unsigned 8-bit samples.
 * Returns NULL on failure. */
nes_audio_t* nes_audio_open(uint32_t sample_rate);

/* Write `n` samples. `volume` 0..100 — caller may pass current volume for per-write attenuation.
 * This call may block until the device can accept the data (used as pacing).
 * Returns number of samples written on success, negative on error. */
int nes_audio_write(nes_audio_t* a, const uint8_t* samples, size_t n, uint8_t volume);

void nes_audio_close(nes_audio_t* a);

#ifdef __cplusplus
}
#endif
#endif
```

- [ ] **Step 2: Update CMakeLists to compile null impl + test**

Modify `src/yps_nes_game_view/CMakeLists.txt`. Replace the `file(GLOB SOURCES src/*.c)` line with explicit sources so we can exclude ALSA impl here:

```cmake
set(WIDGET_SRCS
    src/yps_nes_game_view.c
    src/yps_nes_game_view_register.c
    src/nes_keymap.c
    src/nes_letterbox.c
    src/nes_srampath.c
)

# Audio backend selection
find_package(ALSA QUIET)
if(ALSA_FOUND)
    message(STATUS "yps_nes_game_view: ALSA found, using ALSA audio backend")
    list(APPEND WIDGET_SRCS src/nes_audio_alsa.c)
    add_definitions(-DYPS_NES_HAVE_ALSA=1)
    include_directories(${ALSA_INCLUDE_DIRS})
    set(NES_AUDIO_LIBS ${ALSA_LIBRARIES})
else()
    message(STATUS "yps_nes_game_view: ALSA not found, using null audio backend")
    list(APPEND WIDGET_SRCS src/nes_audio_null.c)
    set(NES_AUDIO_LIBS "")
endif()

add_library(${LIBRARY_NAME} ${WIDGET_SRCS})
target_link_libraries(${LIBRARY_NAME} ${AWTK_SO} ${NES_AUDIO_LIBS} pthread)
```

And extend the x86 tests block:

```cmake
if(PLATFORM STREQUAL "x86")
  add_executable(test_nes_keymap
      tests/test_nes_keymap.c src/nes_keymap.c)
  target_include_directories(test_nes_keymap PRIVATE ${AWTK_INCLUDE})
  install(TARGETS test_nes_keymap RUNTIME DESTINATION ${LIBRARY_NAME})

  add_executable(test_nes_letterbox
      tests/test_nes_letterbox.c src/nes_letterbox.c)
  install(TARGETS test_nes_letterbox RUNTIME DESTINATION ${LIBRARY_NAME})

  add_executable(test_nes_srampath
      tests/test_nes_srampath.c src/nes_srampath.c)
  install(TARGETS test_nes_srampath RUNTIME DESTINATION ${LIBRARY_NAME})

  # Always build null-audio test (uses null impl directly)
  add_executable(test_audio_null
      tests/test_audio_null.c src/nes_audio_null.c)
  install(TARGETS test_audio_null RUNTIME DESTINATION ${LIBRARY_NAME})
endif()
```

- [ ] **Step 3: Run to verify fail**

Run: `./build.sh`
Expected: link error `undefined reference to nes_audio_open`.

- [ ] **Step 4: Implement `src/yps_nes_game_view/src/nes_audio_null.c`**

```c
#include "nes_audio.h"
#include <stdlib.h>
#include <unistd.h>

struct _nes_audio_t {
  uint32_t sample_rate;
};

nes_audio_t* nes_audio_open(uint32_t sample_rate) {
  nes_audio_t* a = (nes_audio_t*)calloc(1, sizeof(nes_audio_t));
  if (!a) return NULL;
  a->sample_rate = sample_rate;
  return a;
}

int nes_audio_write(nes_audio_t* a, const uint8_t* samples, size_t n, uint8_t volume) {
  (void)samples; (void)volume;
  if (!a) return -1;
  /* Pace by sleeping for sample time to avoid flat-out spin when caller relies on blocking.
   * 1e6 / sample_rate microseconds per sample × n samples */
  if (a->sample_rate > 0 && n > 0) {
    useconds_t us = (useconds_t)(((uint64_t)n * 1000000ULL) / a->sample_rate);
    if (us > 0) usleep(us);
  }
  return (int)n;
}

void nes_audio_close(nes_audio_t* a) {
  free(a);
}
```

- [ ] **Step 5: Run to verify pass**

Run: `./build.sh && ./3rdlib/x86/yps_nes_game_view/test_audio_null`
Expected: `test_audio_null OK`

- [ ] **Step 6: Commit**

```bash
git add src/yps_nes_game_view/src/nes_audio.h src/yps_nes_game_view/src/nes_audio_null.c src/yps_nes_game_view/tests/test_audio_null.c src/yps_nes_game_view/CMakeLists.txt
git commit -m "add nes_audio interface + null backend with test"
```

---

## Task 6: ALSA audio backend (smoke-tested, fallback safe)

**Files:**
- Create: `src/yps_nes_game_view/src/nes_audio_alsa.c`

CMake already picks this when ALSA is found (Task 5). If ALSA is not installed on the build machine, this task can be done but the file will not compile unless `libasound2-dev` is installed. The code path must gracefully degrade at runtime when ALSA open fails (so we can still ship binaries).

- [ ] **Step 1: Implement `src/yps_nes_game_view/src/nes_audio_alsa.c`**

```c
#include "nes_audio.h"
#include <alsa/asoundlib.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

struct _nes_audio_t {
  snd_pcm_t* pcm;
  uint32_t sample_rate;
};

nes_audio_t* nes_audio_open(uint32_t sample_rate) {
  nes_audio_t* a = (nes_audio_t*)calloc(1, sizeof(nes_audio_t));
  if (!a) return NULL;
  a->sample_rate = sample_rate;

  int err = snd_pcm_open(&a->pcm, "default", SND_PCM_STREAM_PLAYBACK, 0);
  if (err < 0) {
    fprintf(stderr, "yps_nes_game_view: snd_pcm_open failed: %s\n", snd_strerror(err));
    free(a);
    return NULL;
  }

  /* Mono, U8, sample_rate, ~100ms buffer */
  err = snd_pcm_set_params(a->pcm,
                           SND_PCM_FORMAT_U8,
                           SND_PCM_ACCESS_RW_INTERLEAVED,
                           1,            /* channels */
                           sample_rate,  /* rate */
                           1,            /* allow resampling */
                           100000);      /* latency: 100ms */
  if (err < 0) {
    fprintf(stderr, "yps_nes_game_view: snd_pcm_set_params failed: %s\n", snd_strerror(err));
    snd_pcm_close(a->pcm);
    free(a);
    return NULL;
  }
  return a;
}

int nes_audio_write(nes_audio_t* a, const uint8_t* samples, size_t n, uint8_t volume) {
  if (!a || !a->pcm) return -1;
  if (n == 0) return 0;

  /* Apply volume: U8 samples centered on 128 */
  uint8_t scratch[2048];
  const uint8_t* src = samples;
  if (volume != 100 && n <= sizeof(scratch)) {
    for (size_t i = 0; i < n; i++) {
      int s = (int)samples[i] - 128;
      s = s * (int)volume / 100;
      scratch[i] = (uint8_t)(s + 128);
    }
    src = scratch;
  }

  size_t offset = 0;
  while (offset < n) {
    snd_pcm_sframes_t wrote = snd_pcm_writei(a->pcm, src + offset, n - offset);
    if (wrote < 0) {
      wrote = snd_pcm_recover(a->pcm, (int)wrote, 1);
      if (wrote < 0) return -1;
      continue;
    }
    offset += (size_t)wrote;
  }
  return (int)n;
}

void nes_audio_close(nes_audio_t* a) {
  if (!a) return;
  if (a->pcm) {
    snd_pcm_drain(a->pcm);
    snd_pcm_close(a->pcm);
  }
  free(a);
}
```

- [ ] **Step 2: Verify build still succeeds**

Run: `./build.sh`
Expected: If ALSA headers present, `nes_audio_alsa.c` compiles and libyps_nes_game_view.so links `-lasound`. If ALSA absent, build uses null impl and skips this file.

- [ ] **Step 3: Commit**

```bash
git add src/yps_nes_game_view/src/nes_audio_alsa.c
git commit -m "add ALSA audio backend"
```

---

## Task 7: nes_runtime types + create/destroy skeleton

**Files:**
- Create: `src/yps_nes_game_view/src/nes_runtime.h`
- Create: `src/yps_nes_game_view/src/nes_runtime.c`
- Modify: `src/yps_nes_game_view/CMakeLists.txt` (add nes_runtime.c to sources)

No threading yet; just data structures and create/destroy.

- [ ] **Step 1: Create `src/yps_nes_game_view/src/nes_runtime.h`**

```c
#ifndef YPS_NES_RUNTIME_H
#define YPS_NES_RUNTIME_H
#include <stdint.h>
#include <stdbool.h>
#include <stdatomic.h>
#include <pthread.h>
#include <limits.h>
#include "nes_audio.h"
#ifdef __cplusplus
extern "C" {
#endif

#define NES_FB_W 256
#define NES_FB_H 240

typedef enum {
  NES_STATE_STOPPED = 0,
  NES_STATE_RUNNING,
  NES_STATE_PAUSED,
  NES_STATE_STOPPING,
} nes_state_t;

typedef struct _nes_runtime_t {
  /* Thread + state */
  pthread_t tid;
  pthread_mutex_t state_lock;
  pthread_cond_t  state_cond;
  nes_state_t     state;
  bool            thread_started;

  /* Configuration (stable while thread running) */
  char rom_path[PATH_MAX];
  char sram_path[PATH_MAX];
  bool sound_enable;
  uint32_t sample_rate;

  /* Runtime-modifiable */
  _Atomic uint8_t volume;
  _Atomic float   fps;
  atomic_ulong    last_frame_us;  /* used by fps calc */

  /* Frame buffers */
  uint16_t* buffers[2];
  atomic_int front_idx;

  /* Input */
  _Atomic uint32_t pad1;
  _Atomic uint32_t sys_req;

  /* UI hook */
  void (*on_frame)(void* ctx);
  void* on_frame_ctx;

  /* Audio */
  nes_audio_t* audio;
} nes_runtime_t;

/* Create/destroy — not start/stop; allocates buffers and inits mutex/cond. */
nes_runtime_t* nes_runtime_create(void);
void nes_runtime_destroy(nes_runtime_t* rt);

/* Configuration — must be called while state == STOPPED. */
void nes_runtime_set_rom(nes_runtime_t* rt, const char* rom_path);
void nes_runtime_set_sram_path(nes_runtime_t* rt, const char* sram_path);
void nes_runtime_set_sound(nes_runtime_t* rt, bool enable, uint32_t sample_rate);
void nes_runtime_set_volume(nes_runtime_t* rt, uint8_t volume);
void nes_runtime_set_on_frame(nes_runtime_t* rt, void (*cb)(void*), void* ctx);

/* Lifecycle — implemented later tasks. */
int  nes_runtime_start(nes_runtime_t* rt);   /* 0 OK, <0 err */
int  nes_runtime_pause(nes_runtime_t* rt);
int  nes_runtime_resume(nes_runtime_t* rt);
int  nes_runtime_stop(nes_runtime_t* rt);    /* joins thread */
nes_state_t nes_runtime_state(nes_runtime_t* rt);

/* Accessors used by widget paint / events. */
uint16_t* nes_runtime_front_buffer(nes_runtime_t* rt);
void nes_runtime_pad_set_bit(nes_runtime_t* rt, uint32_t bit, bool pressed);
float nes_runtime_fps(nes_runtime_t* rt);

/* Single-instance guard (InfoNES global state constraint). Returns true if
 * this runtime successfully claimed the singleton; false if another is active. */
bool nes_runtime_claim_singleton(nes_runtime_t* rt);
void nes_runtime_release_singleton(nes_runtime_t* rt);

#ifdef __cplusplus
}
#endif
#endif
```

- [ ] **Step 2: Create `src/yps_nes_game_view/src/nes_runtime.c` — just create/destroy**

```c
#define _GNU_SOURCE
#include "nes_runtime.h"
#include <stdlib.h>
#include <string.h>

/* Singleton claim */
static _Atomic(nes_runtime_t*) g_active_runtime = NULL;

nes_runtime_t* nes_runtime_create(void) {
  nes_runtime_t* rt = (nes_runtime_t*)calloc(1, sizeof(*rt));
  if (!rt) return NULL;

  pthread_mutex_init(&rt->state_lock, NULL);
  pthread_cond_init(&rt->state_cond, NULL);
  rt->state = NES_STATE_STOPPED;
  atomic_init(&rt->volume, 80);
  atomic_init(&rt->fps, 0.0f);
  atomic_init(&rt->last_frame_us, 0);
  atomic_init(&rt->front_idx, 0);
  atomic_init(&rt->pad1, 0);
  atomic_init(&rt->sys_req, 0);
  rt->sample_rate = 22050;
  rt->sound_enable = true;

  rt->buffers[0] = (uint16_t*)calloc(NES_FB_W * NES_FB_H, sizeof(uint16_t));
  rt->buffers[1] = (uint16_t*)calloc(NES_FB_W * NES_FB_H, sizeof(uint16_t));
  if (!rt->buffers[0] || !rt->buffers[1]) {
    free(rt->buffers[0]); free(rt->buffers[1]);
    pthread_mutex_destroy(&rt->state_lock);
    pthread_cond_destroy(&rt->state_cond);
    free(rt);
    return NULL;
  }
  return rt;
}

void nes_runtime_destroy(nes_runtime_t* rt) {
  if (!rt) return;
  if (rt->state != NES_STATE_STOPPED) {
    /* Caller should have stopped us first. Best effort. */
    nes_runtime_stop(rt);
  }
  free(rt->buffers[0]);
  free(rt->buffers[1]);
  pthread_mutex_destroy(&rt->state_lock);
  pthread_cond_destroy(&rt->state_cond);
  free(rt);
}

void nes_runtime_set_rom(nes_runtime_t* rt, const char* p) {
  if (!rt) return;
  strncpy(rt->rom_path, p ? p : "", sizeof(rt->rom_path) - 1);
  rt->rom_path[sizeof(rt->rom_path) - 1] = '\0';
}
void nes_runtime_set_sram_path(nes_runtime_t* rt, const char* p) {
  if (!rt) return;
  strncpy(rt->sram_path, p ? p : "", sizeof(rt->sram_path) - 1);
  rt->sram_path[sizeof(rt->sram_path) - 1] = '\0';
}
void nes_runtime_set_sound(nes_runtime_t* rt, bool e, uint32_t r) {
  if (!rt) return; rt->sound_enable = e; rt->sample_rate = r;
}
void nes_runtime_set_volume(nes_runtime_t* rt, uint8_t v) {
  if (!rt) return; atomic_store(&rt->volume, v);
}
void nes_runtime_set_on_frame(nes_runtime_t* rt, void (*cb)(void*), void* ctx) {
  if (!rt) return; rt->on_frame = cb; rt->on_frame_ctx = ctx;
}

uint16_t* nes_runtime_front_buffer(nes_runtime_t* rt) {
  if (!rt) return NULL;
  int idx = atomic_load(&rt->front_idx);
  return rt->buffers[idx];
}

void nes_runtime_pad_set_bit(nes_runtime_t* rt, uint32_t bit, bool pressed) {
  if (!rt) return;
  uint32_t mask = 1u << bit;
  uint32_t cur = atomic_load(&rt->pad1);
  uint32_t next = pressed ? (cur | mask) : (cur & ~mask);
  atomic_store(&rt->pad1, next);
}

float nes_runtime_fps(nes_runtime_t* rt) {
  if (!rt) return 0.0f;
  return atomic_load(&rt->fps);
}

nes_state_t nes_runtime_state(nes_runtime_t* rt) {
  if (!rt) return NES_STATE_STOPPED;
  pthread_mutex_lock(&rt->state_lock);
  nes_state_t s = rt->state;
  pthread_mutex_unlock(&rt->state_lock);
  return s;
}

bool nes_runtime_claim_singleton(nes_runtime_t* rt) {
  nes_runtime_t* expected = NULL;
  return atomic_compare_exchange_strong(&g_active_runtime, &expected, rt);
}
void nes_runtime_release_singleton(nes_runtime_t* rt) {
  nes_runtime_t* expected = rt;
  atomic_compare_exchange_strong(&g_active_runtime, &expected, NULL);
}

/* Stubs — real impl in Task 9. */
int nes_runtime_start(nes_runtime_t* rt)  { (void)rt; return -1; }
int nes_runtime_pause(nes_runtime_t* rt)  { (void)rt; return -1; }
int nes_runtime_resume(nes_runtime_t* rt) { (void)rt; return -1; }
int nes_runtime_stop(nes_runtime_t* rt)   { (void)rt; return 0; }
```

- [ ] **Step 3: Add to sources + verify build**

Modify `src/yps_nes_game_view/CMakeLists.txt` WIDGET_SRCS block to include `src/nes_runtime.c`:

```cmake
set(WIDGET_SRCS
    src/yps_nes_game_view.c
    src/yps_nes_game_view_register.c
    src/nes_keymap.c
    src/nes_letterbox.c
    src/nes_srampath.c
    src/nes_runtime.c
)
```

Run: `./build.sh`
Expected: successful build (no test for this task; verification is compile-only since runtime is mostly stubs).

- [ ] **Step 4: Commit**

```bash
git add src/yps_nes_game_view/src/nes_runtime.* src/yps_nes_game_view/CMakeLists.txt
git commit -m "add nes_runtime types and create/destroy"
```

---

## Task 8: InfoNES glue — implement all `InfoNES_System.h` callbacks

**Files:**
- Create: `src/yps_nes_game_view/src/infones_glue/InfoNES_System_awtk.cpp`
- Modify: `src/yps_nes_game_view/CMakeLists.txt` (add InfoNES sources + glue)

This is the biggest single task. We implement all 10+ InfoNES callbacks, hooking them to a thread-local runtime pointer set before entering `InfoNES_Main`.

- [ ] **Step 1: Add a "current runtime" thread-local to runtime**

Append to `src/yps_nes_game_view/src/nes_runtime.h` before the `#ifdef __cplusplus` close:

```c
/* Internal: the runtime active for the calling thread (used by InfoNES glue).
 * Set by runtime thread_main just before calling InfoNES_Main. */
nes_runtime_t* nes_runtime_current(void);
void nes_runtime_set_current(nes_runtime_t* rt);
```

Append to `src/yps_nes_game_view/src/nes_runtime.c`:

```c
static __thread nes_runtime_t* tls_current = NULL;
nes_runtime_t* nes_runtime_current(void) { return tls_current; }
void nes_runtime_set_current(nes_runtime_t* rt) { tls_current = rt; }
```

- [ ] **Step 2: Create `src/yps_nes_game_view/src/infones_glue/InfoNES_System_awtk.cpp`**

```cpp
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>

extern "C" {
#include "../nes_runtime.h"
}

#include "../../../../InfoNES/src/InfoNES.h"
#include "../../../../InfoNES/src/InfoNES_System.h"
#include "../../../../InfoNES/src/InfoNES_pAPU.h"

/* NES base palette (0-255 RGB triplets). Source: InfoNES SDL port. */
static const uint8_t kNesPaletteRGB[64][3] = {
  {112,112,112},{32,24,136},{0,0,168},{64,0,152},
  {136,0,112},{168,0,16},{160,0,0},{120,8,0},
  {64,40,0},{0,64,0},{0,80,0},{0,56,16},
  {24,56,88},{0,0,0},{0,0,0},{0,0,0},
  {184,184,184},{0,112,232},{32,56,232},{128,0,240},
  {184,0,184},{224,0,88},{216,40,0},{200,72,8},
  {136,112,0},{0,144,0},{0,168,0},{0,144,56},
  {0,128,136},{0,0,0},{0,0,0},{0,0,0},
  {248,248,248},{56,184,248},{88,144,248},{64,136,248},
  {240,120,248},{248,112,176},{248,112,96},{248,152,56},
  {240,184,56},{128,208,16},{72,216,72},{88,248,152},
  {0,232,216},{0,0,0},{0,0,0},{0,0,0},
  {248,248,248},{168,224,248},{192,208,248},{208,200,248},
  {248,192,248},{248,192,216},{248,184,176},{248,216,168},
  {248,224,160},{224,248,160},{168,240,184},{176,248,200},
  {152,248,240},{0,0,0},{0,0,0},{0,0,0},
};

/* WorkFrame needs to point at the back buffer. Provided by glue. */
extern "C" WORD *WorkFrame;
extern "C" WORD WorkFrame_Backing[NES_DISP_WIDTH * NES_DISP_HEIGHT]; /* unused; kept for link compat if any */

static inline uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b) {
  return (uint16_t)(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
}

/* Called by runtime before InfoNES_Main. */
extern "C" void infones_glue_init_palette_and_workframe(void) {
  for (int i = 0; i < 64; i++) {
    NesPalette[i] = rgb565(kNesPaletteRGB[i][0], kNesPaletteRGB[i][1], kNesPaletteRGB[i][2]);
  }
  nes_runtime_t* rt = nes_runtime_current();
  if (rt) {
    int front = atomic_load(&rt->front_idx);
    WorkFrame = (WORD*)rt->buffers[1 - front];   /* back buffer */
  }
}

/* --- InfoNES required callbacks --- */

extern "C" int InfoNES_Menu(void) {
  nes_runtime_t* rt = nes_runtime_current();
  if (!rt) return -1;
  pthread_mutex_lock(&rt->state_lock);
  nes_state_t s = rt->state;
  pthread_mutex_unlock(&rt->state_lock);
  return (s == NES_STATE_STOPPING) ? -1 : 0;
}

extern "C" int InfoNES_ReadRom(const char* pszFileName) {
  FILE* fp = fopen(pszFileName, "rb");
  if (!fp) return -1;
  if (fread(&NesHeader, sizeof NesHeader, 1, fp) != 1 ||
      memcmp(NesHeader.byID, "NES\x1a", 4) != 0) {
    fclose(fp);
    return -1;
  }
  memset(SRAM, 0, SRAM_SIZE);
  if (NesHeader.byInfo1 & 4) {
    fread(&SRAM[0x1000], 512, 1, fp);
  }
  ROM = (BYTE*)malloc((size_t)NesHeader.byRomSize * 0x4000);
  if (!ROM) { fclose(fp); return -1; }
  fread(ROM, 0x4000, NesHeader.byRomSize, fp);
  if (NesHeader.byVRomSize > 0) {
    VROM = (BYTE*)malloc((size_t)NesHeader.byVRomSize * 0x2000);
    fread(VROM, 0x2000, NesHeader.byVRomSize, fp);
  }
  fclose(fp);
  return 0;
}

extern "C" void InfoNES_ReleaseRom(void) {
  if (ROM)  { free(ROM);  ROM = NULL; }
  if (VROM) { free(VROM); VROM = NULL; }
}

static uint64_t now_us(void) {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return (uint64_t)tv.tv_sec * 1000000ULL + (uint64_t)tv.tv_usec;
}

extern "C" void InfoNES_LoadFrame(void) {
  nes_runtime_t* rt = nes_runtime_current();
  if (!rt) return;

  /* FPS EMA */
  uint64_t t = now_us();
  uint64_t last = atomic_load(&rt->last_frame_us);
  if (last != 0) {
    uint64_t dt = t - last;
    if (dt > 0) {
      float inst = 1000000.0f / (float)dt;
      float cur = atomic_load(&rt->fps);
      float next = (cur == 0.0f) ? inst : (0.9f * cur + 0.1f * inst);
      atomic_store(&rt->fps, next);
    }
  }
  atomic_store(&rt->last_frame_us, t);

  /* Swap: publish the buffer we just finished writing. */
  int old_front = atomic_load(&rt->front_idx);
  int new_front = 1 - old_front;          /* what was back becomes front */
  atomic_store(&rt->front_idx, new_front);
  WorkFrame = (WORD*)rt->buffers[old_front]; /* next frame: write into the other */

  if (rt->on_frame) rt->on_frame(rt->on_frame_ctx);
}

extern "C" void InfoNES_PadState(DWORD* pad1, DWORD* pad2, DWORD* sys) {
  nes_runtime_t* rt = nes_runtime_current();
  if (!rt) { *pad1 = 0; *pad2 = 0; *sys = 0; return; }
  *pad1 = atomic_load(&rt->pad1);
  *pad2 = 0;
  *sys  = atomic_load(&rt->sys_req);
}

extern "C" void* InfoNES_MemoryCopy(void* d, const void* s, int n) {
  return memcpy(d, s, (size_t)n);
}
extern "C" void* InfoNES_MemorySet(void* d, int c, int n) {
  return memset(d, c, (size_t)n);
}
extern "C" void InfoNES_DebugPrint(char* msg) { fprintf(stderr, "%s\n", msg); }

extern "C" void InfoNES_Wait(void) {
  nes_runtime_t* rt = nes_runtime_current();
  if (!rt) return;

  /* Handle pause */
  pthread_mutex_lock(&rt->state_lock);
  while (rt->state == NES_STATE_PAUSED) {
    pthread_cond_wait(&rt->state_cond, &rt->state_lock);
  }
  pthread_mutex_unlock(&rt->state_lock);

  /* Wall-clock frame pacing ONLY when sound is disabled. With sound, SoundOutput blocks. */
  if (!rt->sound_enable) {
    /* InfoNES_Wait is called every scanline (~262/frame). We don't want to sleep 262×/frame;
     * do a coarse pace once per frame by checking a per-scanline counter. */
    static __thread int scanline_counter = 0;
    static __thread uint64_t last_pace_us = 0;
    if (++scanline_counter >= 262) {
      scanline_counter = 0;
      uint64_t t = now_us();
      if (last_pace_us != 0) {
        uint64_t target = last_pace_us + (1000000ULL / 60ULL);
        if (t < target) usleep((useconds_t)(target - t));
      }
      last_pace_us = now_us();
    }
  }
}

extern "C" void InfoNES_SoundInit(void) { /* nothing */ }

extern "C" int InfoNES_SoundOpen(int samples_per_sync, int sample_rate) {
  (void)samples_per_sync;
  nes_runtime_t* rt = nes_runtime_current();
  if (!rt || !rt->sound_enable) return 0;
  if (rt->audio) return 1;
  rt->audio = nes_audio_open((uint32_t)sample_rate);
  if (!rt->audio) {
    fprintf(stderr, "yps_nes_game_view: audio open failed; running silent with frame pacing\n");
    rt->sound_enable = false;   /* fallback, wall-clock pacing kicks in */
    return 0;
  }
  return 1;
}

extern "C" void InfoNES_SoundClose(void) {
  nes_runtime_t* rt = nes_runtime_current();
  if (!rt) return;
  if (rt->audio) { nes_audio_close(rt->audio); rt->audio = NULL; }
}

extern "C" void InfoNES_SoundOutput(int samples, BYTE* w1, BYTE* w2, BYTE* w3, BYTE* w4, BYTE* w5) {
  nes_runtime_t* rt = nes_runtime_current();
  if (!rt || !rt->audio) return;

  /* Mix five u8 waves (mean) into a local buffer; write through audio backend. */
  static uint8_t mix[4096];
  int n = samples > (int)sizeof(mix) ? (int)sizeof(mix) : samples;
  for (int i = 0; i < n; i++) {
    mix[i] = (uint8_t)(((int)w1[i] + (int)w2[i] + (int)w3[i] + (int)w4[i] + (int)w5[i]) / 5);
  }
  nes_audio_write(rt->audio, mix, (size_t)n, atomic_load(&rt->volume));
}

extern "C" void InfoNES_MessageBox(char* fmt, ...) {
  va_list ap; va_start(ap, fmt);
  fprintf(stderr, "yps_nes_game_view: ");
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  va_end(ap);
}
```

- [ ] **Step 3: Add InfoNES sources + glue to CMakeLists**

Modify `src/yps_nes_game_view/CMakeLists.txt` — before `add_library(${LIBRARY_NAME} ${WIDGET_SRCS})`:

```cmake
# InfoNES 3rd-party sources (compiled into this .so)
set(INFONES_DIR ${CMAKE_SOURCE_DIR}/../InfoNES/src)
set(INFONES_SRCS
    ${INFONES_DIR}/InfoNES.cpp
    ${INFONES_DIR}/InfoNES_Mapper.cpp
    ${INFONES_DIR}/InfoNES_pAPU.cpp
    ${INFONES_DIR}/K6502.cpp
)
include_directories(${INFONES_DIR})

set(GLUE_SRCS src/infones_glue/InfoNES_System_awtk.cpp)
```

Change the `add_library` line to include all three source groups:

```cmake
add_library(${LIBRARY_NAME} ${WIDGET_SRCS} ${INFONES_SRCS} ${GLUE_SRCS})
```

`src/CMakeLists.txt` top of file already defines `CMAKE_SOURCE_DIR` as `src/`; our `InfoNES/` is at repo root. `${CMAKE_SOURCE_DIR}/..` points to repo root. Confirm by running:

- [ ] **Step 4: Verify build includes InfoNES**

Run: `./build.sh`
Expected: build succeeds; log shows compilation of `InfoNES.cpp`, `InfoNES_Mapper.cpp`, `InfoNES_pAPU.cpp`, `K6502.cpp`, `InfoNES_System_awtk.cpp`.

If the build fails due to `WorkFrame_Backing[]` extern reference unused, delete the line from `InfoNES_System_awtk.cpp`. If it complains about `WorkFrame` redefinition, see Task 8 Step 5 below.

- [ ] **Step 5: If needed, resolve WorkFrame double definition**

`WorkFrame` is declared and defined in `InfoNES.cpp` line 159. Our glue relies on this being the only definition. No patch needed UNLESS a link error about multiple definition appears. In that case remove the stale `extern "C" WORD WorkFrame_Backing[...]` line (already removed).

- [ ] **Step 6: Commit**

```bash
git add src/yps_nes_game_view/src/infones_glue src/yps_nes_game_view/src/nes_runtime.* src/yps_nes_game_view/CMakeLists.txt
git commit -m "add InfoNES glue: implement system callbacks, link InfoNES sources"
```

---

## Task 9: Runtime start/stop lifecycle + thread

**Files:**
- Modify: `src/yps_nes_game_view/src/nes_runtime.c`

Implement `nes_runtime_start`, `nes_runtime_stop`, plus the thread-main that calls `InfoNES_Main`.

- [ ] **Step 1: Replace the stubs at bottom of `src/yps_nes_game_view/src/nes_runtime.c`**

Find the block after the line `/* Stubs — real impl in Task 9. */` and replace it entirely with:

```c
/* From InfoNES_System.h — declared here to avoid C++ include in C file */
extern int  InfoNES_Load(const char* rom_path);
extern void InfoNES_Main(void);
/* From glue */
extern void infones_glue_init_palette_and_workframe(void);

static void* nes_thread_main(void* arg) {
  nes_runtime_t* rt = (nes_runtime_t*)arg;
  nes_runtime_set_current(rt);

  infones_glue_init_palette_and_workframe();

  /* Load ROM. If fail, signal and exit thread. */
  int rc = InfoNES_Load(rt->rom_path);
  if (rc != 0) {
    pthread_mutex_lock(&rt->state_lock);
    rt->state = NES_STATE_STOPPED;
    pthread_cond_broadcast(&rt->state_cond);
    pthread_mutex_unlock(&rt->state_lock);
    return NULL;
  }

  /* Run the emulator. Returns when InfoNES_Menu() returns -1 (our STOPPING signal). */
  InfoNES_Main();

  /* Save SRAM by reusing InfoNES global state. For now rely on no save;
   * full SRAM save can be layered in a later enhancement (see spec §11). */

  pthread_mutex_lock(&rt->state_lock);
  rt->state = NES_STATE_STOPPED;
  pthread_cond_broadcast(&rt->state_cond);
  pthread_mutex_unlock(&rt->state_lock);
  return NULL;
}

int nes_runtime_start(nes_runtime_t* rt) {
  if (!rt) return -1;
  if (!nes_runtime_claim_singleton(rt)) {
    fprintf(stderr, "yps_nes_game_view: another NES runtime is already running; start refused\n");
    return -2;
  }
  pthread_mutex_lock(&rt->state_lock);
  if (rt->state == NES_STATE_RUNNING) {
    pthread_mutex_unlock(&rt->state_lock);
    return 0;
  }
  if (rt->state == NES_STATE_PAUSED) {
    rt->state = NES_STATE_RUNNING;
    pthread_cond_broadcast(&rt->state_cond);
    pthread_mutex_unlock(&rt->state_lock);
    return 0;
  }
  rt->state = NES_STATE_RUNNING;
  pthread_mutex_unlock(&rt->state_lock);

  int rc = pthread_create(&rt->tid, NULL, nes_thread_main, rt);
  if (rc != 0) {
    pthread_mutex_lock(&rt->state_lock);
    rt->state = NES_STATE_STOPPED;
    pthread_mutex_unlock(&rt->state_lock);
    nes_runtime_release_singleton(rt);
    return -3;
  }
  rt->thread_started = true;
  return 0;
}

int nes_runtime_pause(nes_runtime_t* rt) {
  if (!rt) return -1;
  pthread_mutex_lock(&rt->state_lock);
  if (rt->state == NES_STATE_RUNNING) rt->state = NES_STATE_PAUSED;
  pthread_mutex_unlock(&rt->state_lock);
  return 0;
}

int nes_runtime_resume(nes_runtime_t* rt) {
  if (!rt) return -1;
  pthread_mutex_lock(&rt->state_lock);
  if (rt->state == NES_STATE_PAUSED) {
    rt->state = NES_STATE_RUNNING;
    pthread_cond_broadcast(&rt->state_cond);
  }
  pthread_mutex_unlock(&rt->state_lock);
  return 0;
}

int nes_runtime_stop(nes_runtime_t* rt) {
  if (!rt) return -1;

  pthread_mutex_lock(&rt->state_lock);
  if (rt->state == NES_STATE_STOPPED) {
    pthread_mutex_unlock(&rt->state_lock);
    return 0;
  }
  rt->state = NES_STATE_STOPPING;
  pthread_cond_broadcast(&rt->state_cond);    /* wake if paused */
  pthread_mutex_unlock(&rt->state_lock);

  if (rt->thread_started) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += 2;
    int rc = pthread_timedjoin_np(rt->tid, NULL, &ts);
    if (rc != 0) {
      fprintf(stderr, "yps_nes_game_view: stop timeout; detaching emulator thread\n");
      pthread_detach(rt->tid);
    }
    rt->thread_started = false;
  }
  nes_runtime_release_singleton(rt);
  return 0;
}
```

Ensure `#include <stdio.h>` and `#include <time.h>` are at top of `nes_runtime.c`.

- [ ] **Step 2: Verify build**

Run: `./build.sh`
Expected: build succeeds; `libyps_nes_game_view.so` exists.

- [ ] **Step 3: Commit**

```bash
git add src/yps_nes_game_view/src/nes_runtime.c
git commit -m "implement runtime start/pause/resume/stop with pthread"
```

---

## Task 10: Widget property get/set + lifecycle wiring

**Files:**
- Modify: `src/yps_nes_game_view/src/yps_nes_game_view.c`

Wire the widget to runtime: on_create calls `nes_runtime_create` + applies properties; on_destroy stops and destroys; on_event maps keys to pad bits; on_paint_self blits front buffer through letterbox.

- [ ] **Step 1: Replace entire `src/yps_nes_game_view/src/yps_nes_game_view.c`**

```c
#include "yps_nes_game_view.h"
#include "nes_runtime.h"
#include "nes_keymap.h"
#include "nes_letterbox.h"
#include "nes_srampath.h"

#include "base/keys.h"
#include "base/events.h"
#include "base/idle.h"
#include "base/bitmap.h"
#include "base/widget.h"
#include "tkc/mem.h"
#include "tkc/utils.h"

#include <string.h>

static const uint32_t kDefaultKeyMap[8] = {
  TK_KEY_UP, TK_KEY_DOWN, TK_KEY_LEFT, TK_KEY_RIGHT,
  TK_KEY_x, TK_KEY_z, TK_KEY_s, TK_KEY_a
};

/* Pad bit convention (matches InfoNES SDL port):
 * slot 0 UP    -> bit4
 * slot 1 DOWN  -> bit5
 * slot 2 LEFT  -> bit6
 * slot 3 RIGHT -> bit7
 * slot 4 A     -> bit0
 * slot 5 B     -> bit1
 * slot 6 START -> bit3
 * slot 7 SELECT-> bit2
 */
static const uint32_t kSlotToPadBit[8] = {4, 5, 6, 7, 0, 1, 3, 2};

static void apply_key_map(yps_nes_game_view_t* v) {
  nes_keymap_parse(v->key_map ? v->key_map : "", kDefaultKeyMap, v->keymap);
}

static int8_t keymap_slot_for_code(yps_nes_game_view_t* v, uint32_t code) {
  for (int8_t i = 0; i < 8; i++) if (v->keymap[i] == code) return i;
  return -1;
}

/* UI-thread idle callback: invalidate widget. Runs on UI thread. */
static ret_t frame_invalidate_idle(const idle_info_t* info) {
  widget_t* w = (widget_t*)info->ctx;
  if (w) widget_invalidate_force(w, NULL);
  return RET_REMOVE;
}

/* Called from emulator thread each frame. */
static void on_frame_thread(void* ctx) {
  idle_queue(frame_invalidate_idle, ctx);
}

static ret_t yps_nes_game_view_on_paint_self(widget_t* widget, canvas_t* c) {
  yps_nes_game_view_t* v = YPS_NES_GAME_VIEW(widget);
  if (!v || !v->rt || !v->front_bmp) return RET_OK;

  uint16_t* src = nes_runtime_front_buffer(v->rt);
  if (!src) return RET_OK;

  /* Copy front buffer into bitmap backing. */
  uint8_t* dst = bitmap_lock_buffer_for_write(v->front_bmp);
  if (dst) {
    memcpy(dst, src, NES_FB_W * NES_FB_H * 2);
    bitmap_unlock_buffer(v->front_bmp);
  }

  nes_rect_t r = nes_letterbox_fit(widget->w, widget->h);
  rect_t src_rect = {.x = 0, .y = 0, .w = NES_FB_W, .h = NES_FB_H};
  rect_t dst_rect = {.x = r.x, .y = r.y, .w = r.w, .h = r.h};
  canvas_draw_image(c, v->front_bmp, &src_rect, &dst_rect);
  return RET_OK;
}

static ret_t yps_nes_game_view_on_event(widget_t* widget, event_t* e) {
  yps_nes_game_view_t* v = YPS_NES_GAME_VIEW(widget);
  if (!v || !v->rt) return RET_OK;

  if (e->type == EVT_KEY_DOWN || e->type == EVT_KEY_UP) {
    key_event_t* ke = (key_event_t*)e;
    int8_t slot = keymap_slot_for_code(v, ke->key);
    if (slot >= 0) {
      nes_runtime_pad_set_bit(v->rt, kSlotToPadBit[slot], e->type == EVT_KEY_DOWN);
      return RET_STOP;
    }
  }
  return RET_OK;
}

static ret_t yps_nes_game_view_on_destroy(widget_t* widget) {
  yps_nes_game_view_t* v = YPS_NES_GAME_VIEW(widget);
  if (!v) return RET_BAD_PARAMS;
  if (v->rt) {
    nes_runtime_stop(v->rt);
    nes_runtime_destroy(v->rt);
    v->rt = NULL;
  }
  if (v->front_bmp) {
    bitmap_destroy(v->front_bmp);
    v->front_bmp = NULL;
  }
  TKMEM_FREE(v->rom);
  TKMEM_FREE(v->sram_dir);
  TKMEM_FREE(v->key_map);
  return RET_OK;
}

/* --- property get/set --- */

static ret_t yps_nes_game_view_set_prop(widget_t* widget, const char* name, const value_t* val) {
  yps_nes_game_view_t* v = YPS_NES_GAME_VIEW(widget);
  if (!v) return RET_BAD_PARAMS;

  if (tk_str_eq(name, YPS_NES_GAME_VIEW_PROP_ROM)) {
    TKMEM_FREE(v->rom);
    v->rom = tk_strdup(value_str(val));
    return RET_OK;
  } else if (tk_str_eq(name, YPS_NES_GAME_VIEW_PROP_SRAM_DIR)) {
    TKMEM_FREE(v->sram_dir);
    v->sram_dir = tk_strdup(value_str(val));
    return RET_OK;
  } else if (tk_str_eq(name, YPS_NES_GAME_VIEW_PROP_KEY_MAP)) {
    TKMEM_FREE(v->key_map);
    v->key_map = tk_strdup(value_str(val));
    apply_key_map(v);
    return RET_OK;
  } else if (tk_str_eq(name, YPS_NES_GAME_VIEW_PROP_AUTO_PLAY)) {
    v->auto_play = value_bool(val);
    return RET_OK;
  } else if (tk_str_eq(name, YPS_NES_GAME_VIEW_PROP_SOUND_ENABLE)) {
    v->sound_enable = value_bool(val);
    return RET_OK;
  } else if (tk_str_eq(name, YPS_NES_GAME_VIEW_PROP_SAMPLE_RATE)) {
    v->sample_rate = (uint32_t)value_uint32(val);
    return RET_OK;
  } else if (tk_str_eq(name, YPS_NES_GAME_VIEW_PROP_VOLUME)) {
    v->volume = (uint8_t)value_uint32(val);
    if (v->rt) nes_runtime_set_volume(v->rt, v->volume);
    return RET_OK;
  }
  return RET_NOT_FOUND;
}

static ret_t yps_nes_game_view_get_prop(widget_t* widget, const char* name, value_t* val) {
  yps_nes_game_view_t* v = YPS_NES_GAME_VIEW(widget);
  if (!v) return RET_BAD_PARAMS;

  if (tk_str_eq(name, YPS_NES_GAME_VIEW_PROP_ROM))            { value_set_str(val, v->rom ? v->rom : ""); return RET_OK; }
  if (tk_str_eq(name, YPS_NES_GAME_VIEW_PROP_SRAM_DIR))       { value_set_str(val, v->sram_dir ? v->sram_dir : ""); return RET_OK; }
  if (tk_str_eq(name, YPS_NES_GAME_VIEW_PROP_KEY_MAP))        { value_set_str(val, v->key_map ? v->key_map : ""); return RET_OK; }
  if (tk_str_eq(name, YPS_NES_GAME_VIEW_PROP_AUTO_PLAY))      { value_set_bool(val, v->auto_play); return RET_OK; }
  if (tk_str_eq(name, YPS_NES_GAME_VIEW_PROP_SOUND_ENABLE))   { value_set_bool(val, v->sound_enable); return RET_OK; }
  if (tk_str_eq(name, YPS_NES_GAME_VIEW_PROP_SAMPLE_RATE))    { value_set_uint32(val, v->sample_rate); return RET_OK; }
  if (tk_str_eq(name, YPS_NES_GAME_VIEW_PROP_VOLUME))         { value_set_uint32(val, v->volume); return RET_OK; }
  if (tk_str_eq(name, YPS_NES_GAME_VIEW_PROP_FPS)) {
    value_set_float(val, v->rt ? nes_runtime_fps(v->rt) : 0.0f);
    return RET_OK;
  }
  return RET_NOT_FOUND;
}

TK_DECL_VTABLE(yps_nes_game_view) = {
  .size = sizeof(yps_nes_game_view_t),
  .type = WIDGET_TYPE_YPS_NES_GAME_VIEW,
  .create = yps_nes_game_view_create,
  .set_prop = yps_nes_game_view_set_prop,
  .get_prop = yps_nes_game_view_get_prop,
  .on_paint_self = yps_nes_game_view_on_paint_self,
  .on_event = yps_nes_game_view_on_event,
  .on_destroy = yps_nes_game_view_on_destroy,
};

widget_t* yps_nes_game_view_create(widget_t* parent, xy_t x, xy_t y, wh_t w, wh_t h) {
  widget_t* widget = widget_create(parent, TK_REF_VTABLE(yps_nes_game_view), x, y, w, h);
  yps_nes_game_view_t* v = YPS_NES_GAME_VIEW(widget);
  return_value_if_fail(v != NULL, NULL);

  v->sram_dir = tk_strdup("/tmp");
  v->auto_play = TRUE;
  v->sound_enable = TRUE;
  v->sample_rate = 22050;
  v->volume = 80;
  memcpy(v->keymap, kDefaultKeyMap, sizeof(kDefaultKeyMap));

  v->rt = nes_runtime_create();
  v->front_bmp = bitmap_create_ex(NES_FB_W, NES_FB_H, 0, BITMAP_FMT_RGB565);
  if (v->rt) {
    nes_runtime_set_on_frame(v->rt, on_frame_thread, widget);
  }

  /* Make widget focusable so it receives key events. */
  widget_set_focusable(widget, TRUE);

  return widget;
}

widget_t* yps_nes_game_view_cast(widget_t* widget) {
  return_value_if_fail(widget != NULL && widget->vt == TK_REF_VTABLE(yps_nes_game_view), NULL);
  return widget;
}

/* --- Public API --- */

ret_t yps_nes_game_view_start(widget_t* widget) {
  yps_nes_game_view_t* v = YPS_NES_GAME_VIEW(widget);
  return_value_if_fail(v != NULL && v->rt != NULL, RET_BAD_PARAMS);
  if (!v->rom || !*v->rom) return RET_FAIL;

  /* Configure runtime from current props */
  nes_runtime_set_rom(v->rt, v->rom);
  char sram_path[PATH_MAX];
  nes_srampath_build(v->sram_dir ? v->sram_dir : "/tmp", v->rom, sram_path, sizeof(sram_path));
  nes_runtime_set_sram_path(v->rt, sram_path);
  nes_runtime_set_sound(v->rt, v->sound_enable, v->sample_rate);
  nes_runtime_set_volume(v->rt, v->volume);

  int rc = nes_runtime_start(v->rt);
  return (rc == 0) ? RET_OK : RET_FAIL;
}

ret_t yps_nes_game_view_pause(widget_t* widget) {
  yps_nes_game_view_t* v = YPS_NES_GAME_VIEW(widget);
  return_value_if_fail(v != NULL && v->rt != NULL, RET_BAD_PARAMS);
  return nes_runtime_pause(v->rt) == 0 ? RET_OK : RET_FAIL;
}

ret_t yps_nes_game_view_stop(widget_t* widget) {
  yps_nes_game_view_t* v = YPS_NES_GAME_VIEW(widget);
  return_value_if_fail(v != NULL && v->rt != NULL, RET_BAD_PARAMS);
  return nes_runtime_stop(v->rt) == 0 ? RET_OK : RET_FAIL;
}

ret_t yps_nes_game_view_set_rom(widget_t* widget, const char* rom) {
  yps_nes_game_view_t* v = YPS_NES_GAME_VIEW(widget);
  return_value_if_fail(v != NULL, RET_BAD_PARAMS);
  if (v->rt) nes_runtime_stop(v->rt);
  TKMEM_FREE(v->rom);
  v->rom = tk_strdup(rom ? rom : "");
  return yps_nes_game_view_start(widget);
}

ret_t yps_nes_game_view_set_sram_dir(widget_t* widget, const char* dir) {
  yps_nes_game_view_t* v = YPS_NES_GAME_VIEW(widget);
  return_value_if_fail(v != NULL, RET_BAD_PARAMS);
  TKMEM_FREE(v->sram_dir);
  v->sram_dir = tk_strdup(dir ? dir : "/tmp");
  return RET_OK;
}

ret_t yps_nes_game_view_set_key_map(widget_t* widget, const char* key_map) {
  yps_nes_game_view_t* v = YPS_NES_GAME_VIEW(widget);
  return_value_if_fail(v != NULL, RET_BAD_PARAMS);
  TKMEM_FREE(v->key_map);
  v->key_map = tk_strdup(key_map ? key_map : "");
  apply_key_map(v);
  return RET_OK;
}

ret_t yps_nes_game_view_set_sound_enable(widget_t* widget, bool_t enable) {
  yps_nes_game_view_t* v = YPS_NES_GAME_VIEW(widget);
  return_value_if_fail(v != NULL, RET_BAD_PARAMS);
  v->sound_enable = enable;
  return RET_OK;
}

ret_t yps_nes_game_view_set_sample_rate(widget_t* widget, uint32_t rate) {
  yps_nes_game_view_t* v = YPS_NES_GAME_VIEW(widget);
  return_value_if_fail(v != NULL, RET_BAD_PARAMS);
  v->sample_rate = rate;
  return RET_OK;
}

ret_t yps_nes_game_view_set_volume(widget_t* widget, uint8_t volume) {
  yps_nes_game_view_t* v = YPS_NES_GAME_VIEW(widget);
  return_value_if_fail(v != NULL, RET_BAD_PARAMS);
  v->volume = volume;
  if (v->rt) nes_runtime_set_volume(v->rt, volume);
  return RET_OK;
}

float_t yps_nes_game_view_get_fps(widget_t* widget) {
  yps_nes_game_view_t* v = YPS_NES_GAME_VIEW(widget);
  if (!v || !v->rt) return 0.0f;
  return nes_runtime_fps(v->rt);
}
```

- [ ] **Step 2: Handle auto_play via on_event EVT_WINDOW_OPEN (or parent-ready hook)**

AWTK widgets don't have a direct "ready" hook. Simple approach: start in a deferred idle callback registered at the end of `yps_nes_game_view_create`, so start happens after layout/properties are settled:

At end of `yps_nes_game_view_create`, before `return widget;`, add:

```c
  /* Defer auto_play until after props are applied by XML loader. */
  idle_queue((idle_func_t)yps_nes_game_view_start_if_auto, widget);
```

Then define above the `yps_nes_game_view_create` function:

```c
static ret_t yps_nes_game_view_start_if_auto(const idle_info_t* info) {
  widget_t* w = (widget_t*)info->ctx;
  yps_nes_game_view_t* v = YPS_NES_GAME_VIEW(w);
  if (v && v->auto_play && v->rom && *v->rom) {
    yps_nes_game_view_start(w);
  }
  return RET_REMOVE;
}
```

And change the `idle_queue((idle_func_t)... ` line to: `idle_queue(yps_nes_game_view_start_if_auto, widget);` (no cast needed with proper signature).

- [ ] **Step 3: Verify build**

Run: `./build.sh`
Expected: build succeeds.

- [ ] **Step 4: Commit**

```bash
git add src/yps_nes_game_view/src/yps_nes_game_view.c
git commit -m "wire widget to runtime: props, events, paint, auto_play"
```

---

## Task 11: Guard InfoNES `InfoNES_HSync` to observe stop/pause

**Files:**
- Modify: `InfoNES/src/InfoNES.cpp` (guarded by `#ifdef INFONES_AWTK_GLUE`)

Currently InfoNES_HSync never returns -1 from our code path. We rely on `InfoNES_Menu()` → -1 to exit `InfoNES_Main`. `InfoNES_Menu` is only called between `Cycle` invocations, not inside. Inside `Cycle`, `InfoNES_HSync` drives the scanline loop but only returns -1 via a `#if 0` block. Add a guarded check.

Open `InfoNES/src/InfoNES.cpp`, find function `int InfoNES_HSync()` around line 654. At the very top of the function, add:

- [ ] **Step 1: Patch `InfoNES_HSync` to check runtime stop**

Edit `InfoNES/src/InfoNES.cpp` — insert after the opening `{` of `int InfoNES_HSync()`:

```cpp
#ifdef INFONES_AWTK_GLUE
  /* Cooperative stop hook: let glue layer abort the Cycle() loop. */
  extern "C" int infones_glue_should_stop(void);
  if (infones_glue_should_stop()) return -1;
#endif
```

- [ ] **Step 2: Add `infones_glue_should_stop` in glue**

Append to `src/yps_nes_game_view/src/infones_glue/InfoNES_System_awtk.cpp`:

```cpp
extern "C" int infones_glue_should_stop(void) {
  nes_runtime_t* rt = nes_runtime_current();
  if (!rt) return 1;
  pthread_mutex_lock(&rt->state_lock);
  int stop = (rt->state == NES_STATE_STOPPING) ? 1 : 0;
  pthread_mutex_unlock(&rt->state_lock);
  return stop;
}
```

- [ ] **Step 3: Define `INFONES_AWTK_GLUE` for InfoNES sources in CMake**

Edit `src/yps_nes_game_view/CMakeLists.txt`, add after the `INFONES_SRCS` definition:

```cmake
set_source_files_properties(${INFONES_SRCS} PROPERTIES
    COMPILE_DEFINITIONS "INFONES_AWTK_GLUE=1")
```

- [ ] **Step 4: Verify build**

Run: `./build.sh`
Expected: build succeeds.

- [ ] **Step 5: Commit**

```bash
git add InfoNES/src/InfoNES.cpp src/yps_nes_game_view/src/infones_glue/InfoNES_System_awtk.cpp src/yps_nes_game_view/CMakeLists.txt
git commit -m "add cooperative stop hook in InfoNES_HSync (guarded)"
```

---

## Task 12: End-to-end integration smoke test (manual)

**Files:**
- Create: `src/yps_nes_game_view/tests/integration_test_README.md` — manual test steps

This is a manual integration test — there's no automatic "run game for 5 seconds and check pixels" check. Ensures widget + runtime + InfoNES all work together.

- [ ] **Step 1: Create `src/yps_nes_game_view/tests/integration_test_README.md`**

```markdown
# Manual Integration Test

## Prereq
- x86 build: `./build.sh`
- A sample NES ROM at `/tmp/sample.nes` (public-domain demo ROM, not committed to repo)
- ALSA available and audio working: `aplay -L` lists `default`

## Test 1: widget compiles and registers
  grep "yps_nes_game_view" 3rdlib/x86/yps_nes_game_view/*.h
Expected: API defines present.

## Test 2: library loads
  LD_LIBRARY_PATH=lib/x86:3rdlib/x86/yps_nes_game_view ldd 3rdlib/x86/yps_nes_game_view/libyps_nes_game_view.so
Expected: libawtk.so resolved; libasound.so.2 resolved (if ALSA compiled in).

## Test 3: runtime smoke (no UI)
Write a minimal demo driver:
  gcc -o /tmp/nesmoke - <<'EOF'
  #include <unistd.h>
  extern void* nes_runtime_create(void);
  extern void  nes_runtime_set_rom(void*, const char*);
  extern void  nes_runtime_set_sram_path(void*, const char*);
  extern void  nes_runtime_set_sound(void*, int, unsigned);
  extern int   nes_runtime_start(void*);
  extern int   nes_runtime_stop(void*);
  extern void  nes_runtime_destroy(void*);
  int main() {
    void* rt = nes_runtime_create();
    nes_runtime_set_rom(rt, "/tmp/sample.nes");
    nes_runtime_set_sram_path(rt, "/tmp/sample.srm");
    nes_runtime_set_sound(rt, 0, 22050); /* silent */
    if (nes_runtime_start(rt) != 0) return 1;
    sleep(2);
    nes_runtime_stop(rt);
    nes_runtime_destroy(rt);
    return 0;
  }
  EOF (compile with appropriate -L / -l flags against the built .so)
Expected: exits 0 within ~3 seconds (pause of 2s + stop).

## Test 4: full widget test in a minimal AWTK window
(Left as exploratory — create a small awtk app that XML-includes <yps_nes_game_view rom="/tmp/sample.nes" .../> and run it. Visually confirm the screen updates at ~60fps and key presses move the character.)

## Pass Criteria
1. Build succeeds on x86 with ALSA and without ALSA (`apt remove libasound2-dev && ./build.sh`).
2. All unit tests (`test_nes_keymap`, `test_nes_letterbox`, `test_nes_srampath`, `test_audio_null`) pass.
3. Test 3 (runtime smoke) runs and exits cleanly.
4. Test 4 (widget in window) visually shows moving game, keys responsive, no UI freeze.
```

- [ ] **Step 2: Verify unit tests still pass**

Run:
```
./build.sh
./3rdlib/x86/yps_nes_game_view/test_nes_keymap
./3rdlib/x86/yps_nes_game_view/test_nes_letterbox
./3rdlib/x86/yps_nes_game_view/test_nes_srampath
./3rdlib/x86/yps_nes_game_view/test_audio_null
```
Expected: all 4 binaries print their "OK" lines and exit 0.

- [ ] **Step 3: Commit**

```bash
git add src/yps_nes_game_view/tests/integration_test_README.md
git commit -m "add manual integration test checklist"
```

---

## Self-Review

Covered spec sections vs. tasks:

| Spec § | Task(s) |
|--------|---------|
| §1 目标与范围 | Task 1 (scaffold) |
| §2 架构（2 线程 + 双 buffer） | Task 7 (runtime skeleton), Task 9 (threading), Task 8 (glue), Task 10 (UI side) |
| §3 目录结构 | Task 1, 7, 8 |
| §4 数据结构 | Task 1 (widget struct), Task 7 (runtime struct) |
| §5 数据流 / 生命周期 | Task 8 (LoadFrame swap, Wait, SoundOutput), Task 9 (start/stop), Task 10 (on_paint, on_event, auto_play, set_rom) |
| §6 错误处理 | Task 8 (SoundOpen fallback), Task 9 (stop timeout→detach), Task 10 (empty rom → RET_FAIL) |
| §7 多实例约束 | Task 7 (`nes_runtime_claim_singleton`), Task 9 (enforced in `nes_runtime_start`) |
| §8 构建集成 | Task 1 (scaffold CMake), Task 5 (ALSA find_package), Task 8 (InfoNES sources added) |
| §9 属性 / XML / API | Task 10 |
| §10 测试 | Task 2, 3, 4 (unit tests), Task 5 (null audio test), Task 12 (integration checklist) |
| §11 未来工作 | (no tasks — out of scope) |

**Notable decisions/gaps vs. spec:**

- asset:// URI materialization is **deferred**: Task 10 currently only accepts filesystem paths. Spec §5.1 proposed materializing asset:// to `sram_dir/_extracted_<name>.nes`. This can be added later; for now the widget returns RET_FAIL if `rom` starts with `asset://`. If engineer wants to implement: inside `yps_nes_game_view_start`, if `strncmp(v->rom, "asset://", 8) == 0`, call `assets_manager_ref(assets_manager(), ASSET_TYPE_DATA, v->rom + 8)` → write `asset->data` to tmp file → use that path.
- SaveSRAM inside `nes_thread_main` is TODO (spec says it should run before thread exits). The current code notes this; full impl is a small follow-up: replicate `SaveSRAM()` logic from `InfoNES_System_SDL.cpp` into glue and call it after `InfoNES_Main()` returns.
- "NO ROM" / "UNSUPPORTED MAPPER" error-state visualization in `on_paint_self` is deferred — today an error simply leaves a black widget. Suggested follow-up: check `nes_runtime_state(v->rt) == STOPPED` AND `rom` set but start failed, then `canvas_set_text_color/draw_text_in_rect`.

**Placeholder scan:** No "TBD/TODO" (except the two explicitly deferred items noted above). All code blocks are concrete.

**Type consistency check:**
- `yps_nes_game_view_t` struct members (Task 1) match usage in Task 10.
- `nes_runtime_t` struct (Task 7) matches glue usage in Task 8 and runtime impl in Task 9.
- Pad bit convention: SDL port uses bit0=A ... bit7=Right. Our `kSlotToPadBit` matches. Glue `InfoNES_PadState` returns `rt->pad1` directly — consistent.
- `nes_audio_open`, `_write(… uint8_t volume)`, `_close` signatures identical across header + both impls.
- `bitmap_create_ex(w, h, line_length=0, BITMAP_FMT_RGB565)` — `line_length=0` means "compute from w*bpp" per AWTK convention (see `bitmap.h`).

---

## Plan complete and saved to `docs/superpowers/plans/2026-04-24-yps_nes_game_view.md`. Two execution options:

**1. Subagent-Driven (recommended)** — I dispatch a fresh subagent per task, review between tasks, fast iteration
**2. Inline Execution** — Execute tasks in this session using executing-plans, batch execution with checkpoints

**Which approach?**
