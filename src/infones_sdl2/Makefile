# Standalone SDL2 InfoNES smoke build (x86).
#
# Reuses the InfoNES core that yps_nes_game_view ships under
# src/yps_nes_game_view/libs/ — that prebuilt static archive lets this
# binary build even when depends/InfoNES is absent. To regenerate the
# archive after upgrading InfoNES upstream, run
#   src/yps_nes_game_view/libs/build_libinfones.sh
#
# SDL2: links against the system libSDL2 (apt install libsdl2-dev). The
# locally-built SDL2-2.30.8/install/ ships without the X11 video driver
# (dummy/offscreen only), so a binary linked against it can't open a
# window. Override SDL2_CONFIG to point at a different install if needed.
#
# Output: ../../build/infones_sdl2/infones_sdl2

REPO_ROOT   := $(abspath ../..)
SDL2_CONFIG ?= sdl2-config
PLATFORM    ?= x86
INFONES_PREBUILT_DIR := $(REPO_ROOT)/src/yps_nes_game_view/libs
INFONES_LIB := $(INFONES_PREBUILT_DIR)/$(PLATFORM)/libinfones.a
INFONES_INC := $(INFONES_PREBUILT_DIR)/InfoNES
BUILD_DIR   := $(REPO_ROOT)/build/infones_sdl2
TARGET      := $(BUILD_DIR)/infones_sdl2

SDL2_CFLAGS := $(shell $(SDL2_CONFIG) --cflags)
SDL2_LIBS   := $(shell $(SDL2_CONFIG) --libs)

CXX      ?= g++
CXXFLAGS := -O2 -Wall -Wno-write-strings $(SDL2_CFLAGS) -I$(INFONES_INC) -DINFONES_AWTK_GLUE=1
LDLIBS   := $(INFONES_LIB) $(SDL2_LIBS) -lm

OBJS := $(BUILD_DIR)/InfoNES_System_SDL2.o

.PHONY: all clean

all: $(TARGET)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/InfoNES_System_SDL2.o: InfoNES_System_SDL2.cpp $(INFONES_LIB) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(TARGET): $(OBJS)
	$(CXX) $(OBJS) $(LDLIBS) -o $@

clean:
	rm -rf $(BUILD_DIR)
