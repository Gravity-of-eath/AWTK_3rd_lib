# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

A collection of custom UI widgets (C shared libraries) built on top of the [AWTK](https://github.com/zlgopen/awtk) GUI framework. Each widget is an independent shared library (.so) that links against libawtk.so.

## Build Commands

```bash
# Build for x86 (default)
./build.sh

# Build for a specific target platform
./build.sh t113    # Allwinner T113 (ARM, musl toolchain)
./build.sh t507    # Allwinner T507
./build.sh cv181   # CV181
```

The build script runs CMake with the corresponding toolchain file (`<platform>.cmake`), builds all widget libraries, and installs output to `3rdlib/<platform>/`.

To rebuild from clean state, the script automatically removes and recreates the `build/` directory.

## Architecture

### Widget Layout

Each widget lives under `src/<widget_name>/` with a consistent structure:
- `include/` - Public headers (the widget struct definition + API)
- `src/` - Implementation files
- `CMakeLists.txt` - Builds a shared library named after the widget

The top-level `src/CMakeLists.txt` aggregates all widget subdirectories via `add_subdirectory()`.

### AWTK Widget Pattern

Every widget follows the AWTK custom widget contract:
1. A struct embedding `widget_t` as its first member (enables polymorphic casting)
2. A `*_create()` factory, `*_cast()` cast function, and `*_register()` registration function
3. Property names defined as `#define` constants matching XML attribute names
4. A `TK_EXTERN_VTABLE()` declaration for the widget vtable
5. `BEGIN_C_DECLS`/`END_C_DECLS` guards for C++ compatibility

To integrate a widget into an AWTK app: call `<widget>_register()` at startup, then use the widget type name in UI XML or create it via the C API.

### Platform and Dependencies

- Toolchain files at project root (`x86.cmake`, `t113.cmake`, `t507.cmake`, `cv181.cmake`) define cross-compilation settings
- Pre-built AWTK headers live in `include/<platform>/awtk/src/`
- Pre-built `libawtk.so` lives in `lib/<platform>/`
- Build output (installed .so files + headers) goes to `3rdlib/<platform>/<widget_name>/`

### Widgets

- **line_chart** - Line chart
- **banner_menu** - Animated menu adapter
- **conner_gradient** - Corner gradient (non-GL arc gradient)
- **auto_scale_view** - Container that auto-scales children proportionally
- **blur_view** - Gaussian blur
- **yps_gauge** - Gauge/dial widget with caching strategy for higher frame rates
- **shadow_label** - Reflection/shadow text label
- **breath_ellipse** - Breathing animation ellipse (radial gradient, configurable frequency/scale)
- **yps_gl_view** - OpenGL view based on OGRE 3D engine (includes OGRE headers and libs)
- **recycle_view** - RecyclerView 风格可复用视图容器（adapter + layout manager + 回收池；内置线性/网格布局）

## Testing

The `breath_ellipse` widget has a standalone test executable (`breath_ellipse_view_test`) built from `src/breath_ellipse/tests/`. It tests pure math functions and can be run directly after building.
