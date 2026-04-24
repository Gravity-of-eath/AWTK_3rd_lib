# 当前上下文：yps_nes_game_view 集成
日期: 2026-04-24
状态: 计划阶段完成 — 实施计划已写完并提交，等用户选择执行方式

## 已完成交付物
- 设计文档: docs/superpowers/specs/2026-04-24-yps_nes_game_view-design.md （已提交）
- 实施计划: docs/superpowers/plans/2026-04-24-yps_nes_game_view.md （已提交）
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
- Task 1: Widget 脚手架（空壳）+ 顶层 CMake 集成
- Task 2-4: 单元测试 — keymap parser / letterbox / SRAM path
- Task 5-6: nes_audio null + ALSA 实现
- Task 7: nes_runtime 类型与 create/destroy
- Task 8: InfoNES_System_awtk 所有回调实现
- Task 9: Runtime start/pause/resume/stop 生命周期
- Task 10: Widget 属性/事件/paint/auto_play 串联
- Task 11: InfoNES_HSync 协作停止（#ifdef INFONES_AWTK_GLUE）
- Task 12: 集成手测清单

## 计划中明确 deferred 的项目
- asset:// URI 物化（一期只接 filesystem 路径；实现思路已注明）
- SaveSRAM（在 thread_main 退出前调用，代码来自 SDL 版；留给 follow-up）
- "NO ROM" / "UNSUPPORTED MAPPER" 错误文字占位（黑屏即可）

## 下一步
用户在以下两种执行模式中选一个：
- Subagent-Driven（推荐）: 每任务独立 subagent，任务间 review
- Inline Execution: 本会话 + executing-plans skill 批量执行
