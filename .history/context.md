# 当前上下文：yps_nes_game_view 集成
日期: 2026-04-24
状态: Task 8 完成 — InfoNES_System_awtk glue 实现并与 InfoNES 源码一起链接进 libyps_nes_game_view.so
下一步: Task 9: Runtime start/pause/resume/stop 线程生命周期

## 已完成交付物
- 设计文档: docs/superpowers/specs/2026-04-24-yps_nes_game_view-design.md
- 实施计划: docs/superpowers/plans/2026-04-24-yps_nes_game_view.md
- 任务清单: .history/task.list

## 用户已确认的设计决策（brainstorm 阶段）
1. B 类正经模拟器 UI
2. ROM 通过 widget 属性指定，文件在 `/assets/default/raw/data`
3. 独立 pthread 跑 InfoNES_Main + 双 buffer 原子 swap
4. RGB565 + fit letterbox + nearest（不做 integer-scale）
5. key_map 属性可覆盖默认 `SAXZ+方向键`；只支持 Player 1
6. 控制 API = start/pause/stop/set_rom（不要 reset）
7. sram_dir 属性，默认 /tmp
8. ALSA find_package 自动检测；blocking write 在 emulator 线程内；无声 wall-clock 限帧
9. 附加属性 volume（0~100，默认 80）、fps（只读 EMA）
10. 多实例约束：一个进程仅一个 widget 实例可 start（runtime singleton guard）

## 实施计划概要（12 任务，TDD 顺序）
- Task 1: Widget 脚手架（空壳）+ 顶层 CMake 集成 ✅
- Task 2-4: 单元测试 — keymap parser / letterbox / SRAM path ✅
- Task 5-6: nes_audio null + ALSA 实现 ✅
- Task 7: nes_runtime 类型与 create/destroy ✅ (commit 6a6bab6)
- Task 8: InfoNES_System_awtk 所有回调实现 ✅
- Task 9: Runtime start/pause/resume/stop 生命周期 ⬅ 下一步
- Task 10: Widget 属性/事件/paint/auto_play 串联
- Task 11: InfoNES_HSync 协作停止（#ifdef INFONES_AWTK_GLUE）
- Task 12: 集成手测清单

## Task 8 实现要点
- nes_runtime.h 在 C++ 编译路径下：`#define _Atomic` 为空、提供 `atomic_ulong`/`atomic_int` 的 typedef，
  使结构体布局在 C / C++ 两种模式下一致。C++ 代码用 GCC atomic builtins 访问原子字段。
- InfoNES_System_awtk.cpp 实现 InfoNES_System.h 所有回调：
  Menu/ReadRom/ReleaseRom/LoadFrame/PadState/MemoryCopy/MemorySet/DebugPrint/
  Wait/SoundInit/SoundOpen/SoundClose/SoundOutput/MessageBox。
  另定义 NesPalette[64]（被 InfoNES_System.h 声明为 extern）并在 `infones_glue_init_palette()` 里填 RGB565。
- LoadFrame 采用"拷贝 WorkFrame 到 back buffer，再原子 swap front_idx"策略
  （因为 InfoNES.cpp 中 WorkFrame 是固定数组，不能直接重定向指针）。
- float 原子访问通过 `uint32_t` 别名实现（`__atomic_load_n` 不接受浮点）。
- CMakeLists.txt: 把 InfoNES 4 个核心 cpp (`InfoNES/InfoNES_Mapper/InfoNES_pAPU/K6502`)
  和 glue 文件一起编进 .so。
- 构建验证：`./build.sh` 通过，libyps_nes_game_view.so 成功安装到 3rdlib/x86/yps_nes_game_view/。
  所有 4 个单元测试（keymap / letterbox / srampath / audio_null）继续通过。

## 计划中明确 deferred 的项目
- asset:// URI 物化（一期只接 filesystem 路径；实现思路已注明）
- SaveSRAM（在 thread_main 退出前调用，代码来自 SDL 版；留给 follow-up）
- "NO ROM" / "UNSUPPORTED MAPPER" 错误文字占位（黑屏即可）

## 执行进度
- [x] Task 1 完成（commit 8bd4d4d）
- [x] Task 2 完成（commit 9781415）
- [x] Task 3 完成（commit 6116313）
- [x] Task 4 完成（commit bfd3c32）
- [x] Task 5 完成（commit 69d000d）
- [x] Task 6 完成（commit 3cd6c05）
- [x] Task 7 完成（commit 6a6bab6）
- [x] Task 8 完成（pending commit）

## 下一步
进入 Task 9: Runtime start/pause/resume/stop 生命周期实现
- 在 nes_runtime.c 中替换掉 Task 7 留下的 stub（`-1` 返回）
- 新开 pthread 跑 thread_main：claim singleton → set_current → init_palette → InfoNES_Main
- pause/resume 通过 state+cond 驱动 InfoNES_Wait 里的 wait 循环
- stop 设置 STOPPING 让 InfoNES_Menu 返回 -1 跳出主循环，并 join 线程
