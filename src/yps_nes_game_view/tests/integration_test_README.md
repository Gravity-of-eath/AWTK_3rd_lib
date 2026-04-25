# yps_nes_game_view — Integration Test Checklist

Manual integration tests. There is no "play the game for 5s and assert pixels"
automatic check; the goal is to make sure the widget loads, the runtime starts,
the InfoNES core renders, and a clean shutdown is possible.

## Prerequisites

- x86 build of the project: `./build.sh`
- A clean iNES ROM under `InfoNES/rom/...` (verify with `head -c 4 file.nes | xxd`
  → `4e45 531a` magic, i.e. `NES\x1a`). The library `InfoNES/rom/Contra/` is
  known to load.
- For the standalone reference emulator only: system SDL2 with X11 driver
  (`apt install libsdl2-dev`). Verify with `strings $(ldconfig -p | grep
  libSDL2 | head -1 | awk '{print $4}') | grep -i "^x11$"`.
- For ALSA-enabled builds: working `aplay -L` (lists `default`).

## Test 1 — widget builds and exports its public API

    nm -D 3rdlib/x86/yps_nes_game_view/libyps_nes_game_view.so \
      | grep -E " T yps_nes_game_view_" | wc -l

Expected: ≥ 14 (one per declared API in `include/yps_nes_game_view.h`).

## Test 2 — widget library loads cleanly

    LD_LIBRARY_PATH=lib/x86 \
      ldd 3rdlib/x86/yps_nes_game_view/libyps_nes_game_view.so

Expected: `libawtk.so` resolved, `libpthread.so.0` resolved, `libasound.so.2`
resolved if ALSA was found at configure time. No `not found` lines.

## Test 3 — unit tests still pass

    ./3rdlib/x86/yps_nes_game_view/test_nes_keymap
    ./3rdlib/x86/yps_nes_game_view/test_nes_letterbox
    ./3rdlib/x86/yps_nes_game_view/test_nes_srampath
    ./3rdlib/x86/yps_nes_game_view/test_audio_null

Expected: each binary prints its `OK` lines and exits 0.

## Test 4 — standalone reference emulator (sanity-check InfoNES itself)

The standalone is a minimal SDL2 binary built from the *same* InfoNES core
the widget links. If it works, the core is fine; any later widget bug is in
the AWTK glue, not in InfoNES.

    make -C src/infones_sdl2
    SDL_VIDEODRIVER=x11 \
      build/infones_sdl2/infones_sdl2 'InfoNES/rom/Contra/Contra (U) [!].nes'

Expected: 512×480 window opens with the Contra title sequence.
Keys: arrows = D-pad, X = A, Z = B, Space = Select, Enter = Start, Esc = quit.
Press Esc → window closes, terminal returns to prompt with no zombies
(`ps aux | grep infones_sdl2` empty).

Headless variant (no display required, use to confirm exit semantics in CI):

    SDL_VIDEODRIVER=dummy SDL_RENDER_DRIVER=software timeout 2 \
      build/infones_sdl2/infones_sdl2 'InfoNES/rom/Contra/Contra (U) [!].nes'

Expected: prints `InfoNES running...`, no `[InfoNES]` errors, no SEGV; exit
124 from `timeout`'s SIGTERM after 2s.

## Test 5 — runtime smoke (no AWTK UI)

This exercises `nes_runtime_*` directly, bypassing the widget. Useful when
changes to runtime/glue need to be verified in isolation.

Compile a tiny driver against the widget .so (the runtime symbols are
exported because they live in the same TU):

    cat >/tmp/nesmoke.c <<'EOF'
    #include <unistd.h>
    typedef struct nes_runtime nes_runtime_t;
    extern nes_runtime_t* nes_runtime_create(void);
    extern void nes_runtime_set_rom(nes_runtime_t*, const char*);
    extern void nes_runtime_set_sram_path(nes_runtime_t*, const char*);
    extern void nes_runtime_set_sound(nes_runtime_t*, int, unsigned);
    extern int  nes_runtime_start(nes_runtime_t*);
    extern int  nes_runtime_stop(nes_runtime_t*);
    extern void nes_runtime_destroy(nes_runtime_t*);
    int main(int argc, char** argv) {
      nes_runtime_t* rt = nes_runtime_create();
      nes_runtime_set_rom(rt, argv[1]);
      nes_runtime_set_sram_path(rt, "/tmp/smoke.srm");
      nes_runtime_set_sound(rt, 0, 22050);   /* silent */
      if (nes_runtime_start(rt) != 0) return 1;
      sleep(2);
      nes_runtime_stop(rt);
      nes_runtime_destroy(rt);
      return 0;
    }
    EOF
    gcc -O2 /tmp/nesmoke.c \
        -L 3rdlib/x86/yps_nes_game_view -lyps_nes_game_view \
        -Wl,-rpath,$(pwd)/3rdlib/x86/yps_nes_game_view \
        -o /tmp/nesmoke
    /tmp/nesmoke 'InfoNES/rom/Contra/Contra (U) [!].nes'; echo "exit=$?"

Expected: returns within ~3 s, exit 0, no zombie thread (no leftover
`nesmoke` in `ps aux`).

## Test 6 — full widget in a minimal AWTK window  *(exploratory)*

Out of scope for an automated checklist — left as a hand-test for whoever
integrates this widget. Recipe:

1. Build a small AWTK demo app that includes the widget header and calls
   `yps_nes_game_view_register()` at startup.
2. Add an XML window:
       <window>
         <yps_nes_game_view x="0" y="0" w="100%" h="100%"
             rom="/abs/path/to/rom.nes"
             auto_play="true"
             volume="80"/>
       </window>
3. Run the app. Visually confirm:
   - Game appears, scaled to fit the window with letterbox bars
   - 60 fps (no stutter) — read with `nes_runtime_fps` if needed
   - Arrow keys move the character; X/Z fire/jump
   - Closing the window does not leak the emulator thread
     (`ps -T -p <pid>` after the window closes)

## Pass Criteria

| # | Test                              | Pass condition                                    |
|---|-----------------------------------|---------------------------------------------------|
| 1 | API symbols                       | ≥ 14 `T yps_nes_game_view_*` symbols              |
| 2 | Library loads                     | No `not found` in `ldd` output                    |
| 3 | Unit tests                        | All 4 print `OK`, exit 0                          |
| 4 | Standalone (display + headless)   | Window opens; headless exits cleanly under timeout |
| 5 | Runtime smoke                     | exit 0, no leaked thread                          |
| 6 | Widget in AWTK app                | Hand-eyeball: game runs, keys work, no leak       |

Tests 1–5 should be re-run after every change to the widget, runtime, or
glue. Test 6 is recommended whenever public properties or events change.
