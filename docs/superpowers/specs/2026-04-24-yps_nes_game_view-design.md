# yps_nes_game_view 设计文档

- 日期: 2026-04-24
- 作者: taylor.yao（Claude 协助）
- 状态: Draft（等待 user review）

## 1. 目标与范围

把 [InfoNES](../../../InfoNES) 这款 FC(NES) 模拟器集成到 AWTK 框架中，作为一个可以在任意 AWTK 窗口中嵌入的自定义控件 `yps_nes_game_view`。

**用例类别：正经模拟器 UI（B 类）**
- 用户通过 widget 属性指定 ROM；ROM 文件放在 AWTK asset 目录（如 `/assets/default/raw/data/*.nes`）
- 支持键盘映射、运行时 start/pause/stop、切换 ROM、保存 SRAM
- 声音**非必须**，按平台自动启用 ALSA；无 ALSA 时静默 + 软件限帧
- 一期只支持 Player 1

**非目标：**
- Player 2
- 多 widget 实例并存（InfoNES 全局状态天然单例，一期不解决）
- 网络联机 / rewind / savestate（除 SRAM 以外）
- 音频子系统独立线程（选用 blocking write + emulator 线程同步的简化模型）

## 2. 架构总览

```
┌────────────────────────────────────────────────────────────┐
│                     AWTK UI 线程                            │
│  ┌──────────────────────────────────────────────────────┐  │
│  │ yps_nes_game_view widget                             │  │
│  │   on_paint_self:  front_buffer → canvas (blit+缩放)  │  │
│  │   on_event(KEY):  更新共享 pad_state (atomic)        │  │
│  │   on_destroy:     停线程、释放 buffer                │  │
│  │   API: start/pause/stop/set_rom                      │  │
│  └──────────────────────────────────────────────────────┘  │
│           ▲ invalidate              ▼ pad_state (atomic)   │
└───────────┼──────────────────────────┼─────────────────────┘
            │                          │
┌───────────┴──────────────────────────┴─────────────────────┐
│                Emulator 线程（pthread）                     │
│  InfoNES_Main() loop:                                       │
│    InfoNES_Cycle → InfoNES_LoadFrame (swap back↔front)     │
│                  → InfoNES_PadState (读 atomic pad)         │
│                  → InfoNES_SoundOutput (blocking ALSA)      │
│                  → InfoNES_Wait       (仅无声时 wall-clock) │
└─────────────────────────────────────────────────────────────┘
```

**设计要点：**
- Emulator 跑在独立 pthread；UI 线程和它只通过 3 样东西共享：双 frame buffer（原子 swap）、pad 状态（原子 32-bit）、控制 flag（mutex + cond）
- InfoNES.cpp 几乎不改，主要通过提供新的 `InfoNES_System_awtk.cpp` 实现它声明的系统回调
- `WorkFrame` 直接指向 back_buffer，emulator 把像素直接写到最终要展示的内存，避免 memcpy

## 3. 目录结构

```
src/yps_nes_game_view/
├── CMakeLists.txt
├── include/
│   └── yps_nes_game_view.h          # widget 公开 API + 属性宏
└── src/
    ├── yps_nes_game_view.c          # widget vtable: create/paint/event/destroy
    ├── yps_nes_game_view.h          # widget 内部结构（_t）
    ├── yps_nes_game_view_register.c # 注册函数
    ├── nes_runtime.h / .c           # emulator 线程封装（纯 C 接口）
    ├── nes_audio.h                  # nes_audio_{open,write,close} 接口
    ├── nes_audio_alsa.c             # ALSA blocking 实现（find_package ALSA 才编）
    ├── nes_audio_null.c             # 空实现（无 ALSA 时编）
    └── infones_glue/
        └── InfoNES_System_awtk.cpp  # LoadFrame/PadState/Wait/SoundOutput/... 的实现
```

### 组件职责边界

| 组件 | 做什么 | 依赖谁 | 谁用它 |
|------|--------|--------|--------|
| `yps_nes_game_view.c` | AWTK widget 语义（属性 get/set、paint、event、lifecycle） | `nes_runtime` | AWTK app |
| `nes_runtime` | 纯 C 封装：runtime create/destroy、start/pause/stop/set_rom、取 front buffer、写 pad、订阅"新帧"回调 | `InfoNES_System_awtk`、`nes_audio_*` | widget |
| `InfoNES_System_awtk.cpp` | 实现 InfoNES 要求的系统回调，读写 runtime 共享状态 | runtime 共享数据结构 | 被 InfoNES 直接调 |
| `nes_audio_alsa.c` / `nes_audio_null.c` | blocking 语义的音频输出 | libasound / 空 | `InfoNES_System_awtk` |

**切分理由：**
- widget 层只处理 AWTK 语义，不关心线程和 InfoNES；runtime 是"无 UI 的可跑模拟器"，方便写单元测试
- `infones_glue` 把 InfoNES 的 C++/全局状态污染圈在小区域；runtime 对外是纯 C 接口
- 音频两份实现 + 编译期二选一，无宏散布到业务代码

## 4. 数据结构

### 4.1 Widget 公开结构

```c
typedef struct _yps_nes_game_view_t {
  widget_t widget;
  char* rom;             // ROM 路径或 asset URI，例 "asset://data/mario.nes" 或 "/abs/path.nes"
  char* sram_dir;        // SRAM 存档目录，默认 "/tmp"
  char* key_map;         // "up=UP,down=DOWN,a=X,b=Z,start=S,select=A"（可覆盖默认）
  bool_t auto_play;      // 默认 true
  bool_t sound_enable;   // 默认 true；false 时走 wall-clock 限帧
  uint32_t sample_rate;  // 默认 22050
  uint8_t volume;        // 0~100，默认 80
  // 注：fps 不在 widget 缓存，get_fps 直接从 rt->fps 原子读

  nes_runtime_t* rt;
  bitmap_t* front_bmp;   // 包 front_buffer 以便 canvas_draw_image
  uint32_t keymap[8];    // 解析后的 AWTK keycode 表
                         //   0=UP 1=DOWN 2=LEFT 3=RIGHT 4=A 5=B 6=START 7=SELECT
} yps_nes_game_view_t;
```

### 4.2 Runtime 内部结构

```c
typedef enum {
  NES_STATE_STOPPED = 0,
  NES_STATE_RUNNING,
  NES_STATE_PAUSED,
  NES_STATE_STOPPING,
} nes_state_t;

struct nes_runtime_t {
  pthread_t tid;
  pthread_mutex_t state_lock;
  pthread_cond_t  state_cond;
  nes_state_t     state;

  char rom_path[PATH_MAX];
  char sram_path[PATH_MAX];    // = sram_dir + "/" + basename_no_ext(rom) + ".srm"
  bool_t sound_enable;
  uint32_t sample_rate;
  _Atomic uint8_t volume;       // 0~100
  _Atomic float   fps;

  uint16_t* buffers[2];         // RGB565，256*240*2 = 122880 字节 ×2
  _Atomic int front_idx;        // UI 读 buffers[front_idx]，emulator 写 buffers[1-front_idx]

  _Atomic uint32_t pad1;        // bit0=A bit1=B bit2=Select bit3=Start bit4=U bit5=D bit6=L bit7=R
  _Atomic uint32_t sys_req;     // PAD_SYS_QUIT 等

  void (*on_frame)(void* ctx);
  void* on_frame_ctx;

  nes_audio_t* audio;
};
```

### 4.3 共享访问规则

| 数据 | 写方 | 读方 | 同步手段 |
|------|------|------|---------|
| `buffers[].` 内容 | emulator | UI paint | front_idx 原子切换；写方只写 back buffer |
| `front_idx` | emulator | UI | `_Atomic int`，release/acquire |
| `pad1` / `sys_req` | UI（key event） | emulator | `_Atomic uint32_t` |
| `volume` | UI | emulator（audio write） | `_Atomic uint8_t` |
| `fps` | emulator | UI | `_Atomic float` |
| `state` | UI（start/pause/stop） | emulator | `mutex + cond` |
| `rom_path` | UI（set_rom） | emulator | set_rom 串行化为 stop → 改 → start |

**emulator → UI invalidate 的线程安全：** emulator 线程不能直接调 `widget_invalidate_force`（AWTK 非线程安全）。要通过 `idle_queue(...)` 投递到 UI 线程；AWTK 的 `idle_queue` 线程安全。Widget 层封装这一步。

## 5. 数据流 & 生命周期

### 5.1 启动流程（`start` 或 `auto_play`）

```
UI 线程:
  widget_create → 解析属性 → 分配 front/back buffer（RGB565）
  → 解析 rom 属性并物化为本地文件路径（InfoNES_ReadRom 只接受 filesystem 路径）:
      /abs/path      → 直接用
      asset://xxx    → data_reader_read → 写入 sram_dir/_extracted_<basename>.nes → 用这个临时路径
                       (避免改 InfoNES_ReadRom 的签名；临时文件在 stop() 时删)
  → nes_runtime_start(rt):
      ├ 填 NesPalette[64]（RGB565，from NesPaletteRGB）
      ├ InfoNES_Load(rom_path) 解析 ROM header/ROM/VROM  // rom_path 是物化后的 filesystem 路径
      ├ LoadSRAM()（若 ROM 声明需要 SRAM）
      ├ nes_audio_open(sample_rate)  -- sound_enable=true 才开
      ├ pthread_create(thread_main)
      └ state = RUNNING
      thread_main: InfoNES_Main()（内部 = InfoNES_Init + InfoNES_Cycle 循环）
```

### 5.2 每帧数据流（emulator 线程）

```
InfoNES_Cycle() 每条扫描线:
  K6502_Step → InfoNES_DrawLine → 像素直接写 WorkFrame[line*256..]
  (WorkFrame == back_buffer，像素值 = NesPalette[color] = RGB565，零拷贝)

一帧结束 (InfoNES_LoadFrame):
  ├ compute_fps(rt)                         # wall clock + EMA
  ├ atomic_store(front_idx, 1-front_idx)    # release
  ├ 切换 back_buffer 指向 buffers[1-front_idx]；下一帧 WorkFrame 指向新 back
  └ on_frame(ctx)                           # 内部 idle_queue(invalidate)

InfoNES_PadState()    → atomic_load(pad1 / sys_req)
InfoNES_Wait()        → sound_enable ? no-op : wall-clock usleep 到下一帧
InfoNES_SoundOutput() → nes_audio_write()（blocking，按 volume 衰减）
```

### 5.3 UI paint

```c
on_paint_self(canvas):
  idx = atomic_load(front_idx);  // acquire
  bitmap 用 buffers[idx] (RGB565, 256x240) 包装；
  fit-letterbox 计算 dst_rect:
      scale = min(w/256.0, h/240.0);
      dst_w = 256*scale; dst_h = 240*scale;
      dst_x = (w-dst_w)/2; dst_y = (h-dst_h)/2;
  canvas_draw_image(canvas, bitmap, &src(0,0,256,240), &dst_rect);
  // AWTK 默认最近邻缩放
```

### 5.4 控制操作语义

| API | 实现 | 幂等 |
|-----|------|------|
| `start()` | state=STOPPED/PAUSED → 启/唤线程；RUNNING 时 no-op | 是 |
| `pause()` | state=RUNNING → PAUSED；emulator 在 `InfoNES_HSync` 检查 state，PAUSED 时 cond_wait | 是 |
| `stop()` | state=* → STOPPING；`InfoNES_HSync` 返回 -1 让 Cycle 退出；emulator 线程在返回前自己调 SaveSRAM；UI 线程 pthread_join；再 free ROM / 删临时 ROM 文件 | 是 |
| `set_rom(path)` | stop → 改 rom_path → start；state_lock 串行化 | 串行 |
| `on_destroy` | stop() + 释放 buffer + free runtime | — |

## 6. 错误处理

| 错误场景 | 策略 |
|---------|------|
| ROM 文件打不开 | `start` 返回 `RET_FAIL`；paint 画 "NO ROM" 文字占位 |
| ROM 格式不对 | 同上，走 `InfoNES_Load` 返回码 |
| 不支持的 mapper | InfoNES 自判错；widget 显示 "UNSUPPORTED MAPPER #N" |
| ALSA open 失败 | 日志 warning；fallback 到 silent + wall-clock 限帧 |
| malloc 失败 | 返回 `RET_OOM`，清理已分配资源 |
| SRAM 写失败 | 日志 warning，不影响运行 |
| widget 销毁时线程还在跑 | `on_destroy` 必须 stop + join；stop 用 `pthread_timedjoin_np` 做 2s 超时；超时后仅打 error 日志并 detach 线程（避免阻塞 UI 永久；接受线程泄漏换稳定性） |

## 7. 多实例约束（硬约束）

由于 InfoNES 使用大量全局状态（NesPalette / WorkFrame / RAM / PPURAM / K6502 寄存器 / mapper 上下文 …）：

- **同一进程内同一时刻只允许一个 `yps_nes_game_view` 在运行。**
- Widget create 时检测全局 "is_running" 标志；若已有实例在跑，第二个 widget 拒绝 start（返回 `RET_BUSY` + 日志警告）
- 如果真有"多 widget 同屏显示同一个画面"的需求，可以共享同一个 runtime，多 widget 共享 front_buffer——**一期不做，future work**

## 8. 构建集成

### 8.1 顶层集成

`src/CMakeLists.txt` 追加 `add_subdirectory(yps_nes_game_view)`。

### 8.2 子 CMakeLists.txt（要点）

- `file(GLOB INFONES_SRCS ${CMAKE_SOURCE_DIR}/InfoNES/src/*.cpp)` 直接编进 .so
  - 不编 `InfoNES/src/{linux,sdl,win32,...}` 下的平台 main
- ALSA 用 `find_package(ALSA)` 自动检测：有则定义 `HAVE_ALSA` + 链 `${ALSA_LIBRARIES}` + 编 `nes_audio_alsa.c`；没有就编 `nes_audio_null.c`
- 链 `awtk`、`pthread`

### 8.3 InfoNES 源码的必要修改

原则：能在 glue 里搞定的不碰 InfoNES.cpp；实在要碰用 `#ifdef INFONES_AWTK_GLUE` 包住。

预计需要的改动点（在实现阶段核实）：
- `InfoNES_HSync` 检查 runtime.state：PAUSED 时 cond_wait，STOPPING 时 return -1
- `InfoNES_Init` / `InfoNES_Fin` 确保全局重入安全（start → stop → 再 start 不崩）
- `WorkFrame = DoubleFrame[0]` 这类初始化改为由 glue 层赋值

## 9. 属性 & XML 示例

```xml
<yps_nes_game_view x="0" y="0" w="512" h="480"
    rom="asset://data/mario.nes"
    sram_dir="/tmp"
    sound_enable="true"
    sample_rate="22050"
    volume="80"
    auto_play="true"
    key_map="up=UP,down=DOWN,a=X,b=Z,start=S,select=A"/>
```

**属性表：**

| 属性 | 类型 | 默认 | 备注 |
|------|------|------|------|
| `rom` | str | "" | 支持 `asset://` 或绝对路径 |
| `sram_dir` | str | `/tmp` | SRAM `.srm` 存放目录 |
| `sound_enable` | bool | true | false 时走 wall-clock 限帧 |
| `sample_rate` | u32 | 22050 | Hz |
| `volume` | u8 | 80 | 0~100 |
| `auto_play` | bool | true | 创建后自动 start |
| `key_map` | str | (默认见下) | 可只写差异项 |
| `fps` | float | 0 | **只读** |

**默认 key_map**（与 SDL 版本一致）：`up=UP,down=DOWN,left=LEFT,right=RIGHT,a=X,b=Z,start=S,select=A`

**公开 C API：**

```c
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
```

## 10. 测试策略

**单元测试**（不依赖 AWTK，不依赖 InfoNES 跑起来，参照 `src/breath_ellipse/tests/`）：
- `tests/test_keymap_parse.c` — `"up=UP,down=DOWN,..."` 解析
- `tests/test_letterbox.c` — fit-letterbox dst_rect 计算
- `tests/test_sram_path.c` — `sram_dir + rom basename → .srm` 拼接

**集成手测**：
- x86：`/assets/default/raw/data/sample1.nes`，XML 嵌入 `yps_nes_game_view`，目视画面/手柄/声音
- 无 ALSA 场景：卸载 libasound，重编确认走 silent + 限帧
- 生命周期：`set_rom` 切换、`pause/start` 循环、widget 销毁；valgrind / sanitizer 确认无内存/线程泄漏

**不做：**
- 录像帧对拍
- mapper 覆盖率测试（InfoNES 上游已处理）

## 11. 开放问题与未来工作

- 多 widget 共享同一 runtime（用同一个 front buffer）以绕过单实例约束
- Player 2
- Savestate / rewind
- 音频输出独立线程 + ring buffer（若 blocking 方案在某些平台出问题再考虑）
- `key_map` 支持手柄事件源（目前只接 `EVT_KEY_DOWN/UP`）
