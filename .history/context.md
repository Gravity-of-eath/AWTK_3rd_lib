# 当前上下文：yps_nes_game_view 集成
日期: 2026-04-24
状态: Brainstorm 阶段 — 已写完 design doc，等用户 review

## 用户已确认的设计决策

1. **用例定位**：B 类正经模拟器 UI
2. **ROM 配置**：widget 属性（XML 或 C），文件在 `/assets/default/raw/data`
3. **主循环**：独立 pthread 跑 InfoNES_Main() + 双 buffer（原子 swap）
4. **像素格式**：RGB565；fit letterbox；nearest 缩放；不做 integer-scale
5. **输入**：key_map 属性可覆盖默认 `SAXZ+方向键`，只支持 Player 1
6. **控制 API**：start/pause/stop/set_rom（**不要 reset**）
7. **SRAM**：`sram_dir` 属性，默认 `/tmp`
8. **音频**：CMake find_package(ALSA) 自动检测；ALSA blocking write 在 emulator 线程内（方案 i）；无声走 wall-clock 限帧
9. **附加属性**：`volume`（0~100，默认 80）、`fps`（只读，EMA）
10. **多实例**：硬约束一个进程只能跑一个 widget 实例（InfoNES 全局状态原因）

## 交付物
- 设计文档: docs/superpowers/specs/2026-04-24-yps_nes_game_view-design.md
- 任务清单: .history/task.list

## 下一步
1. 自检 spec 是否有 placeholder/一致性/歧义问题
2. 请求用户 review written spec
3. 进入 writing-plans skill 产出实现计划（不走其他实现类 skill）

## 关键技术细节备忘（避免重开对话丢失）

- `WorkFrame = back_buffer`：emulator 把像素直接生成到 back buffer，swap 时改 WorkFrame 指针
- `on_frame` emulator → UI 的 invalidate 必须走 `idle_queue`（AWTK widget_invalidate 非线程安全）
- InfoNES.cpp 的 patch 用 `#ifdef INFONES_AWTK_GLUE` 包住以便升级
- InfoNES_HSync 在 PAUSED 时 cond_wait；STOPPING 时 return -1 让 Cycle 退出
- NesPalette[64] 用 RGB565 填充（from NesPaletteRGB[64][3]）；WorkFrame 的像素就直接是 RGB565 值
