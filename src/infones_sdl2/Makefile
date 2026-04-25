# Standalone SDL2 InfoNES smoke build (x86).
#
# Uses the system SDL2 (apt install libsdl2-dev) by default — its X11 video
# driver is needed to actually render to an X11 desktop. The locally-built
# SDL2-2.30.8/install/ ships without X11 (dummy/offscreen only) so a binary
# linked against it can't show a window.
# Override SDL2_CONFIG to point at a different install if needed.
#
# Output: ../../build/infones_sdl2/infones_sdl2

REPO_ROOT   := $(abspath ../..)
SDL2_CONFIG ?= sdl2-config
INFONES_DIR := $(REPO_ROOT)/InfoNES/src
BUILD_DIR   := $(REPO_ROOT)/build/infones_sdl2
TARGET      := $(BUILD_DIR)/infones_sdl2

SDL2_CFLAGS := $(shell $(SDL2_CONFIG) --cflags)
SDL2_LIBS   := $(shell $(SDL2_CONFIG) --libs)

CXX      ?= g++
CXXFLAGS := -O2 -Wall -Wno-write-strings $(SDL2_CFLAGS) -I$(INFONES_DIR) -DINFONES_AWTK_GLUE=1
LDLIBS   := $(SDL2_LIBS) -lm

CORE_SRCS := \
    $(INFONES_DIR)/InfoNES.cpp \
    $(INFONES_DIR)/InfoNES_Mapper.cpp \
    $(INFONES_DIR)/InfoNES_pAPU.cpp \
    $(INFONES_DIR)/K6502.cpp

LOCAL_SRC := InfoNES_System_SDL2.cpp

OBJS := \
    $(BUILD_DIR)/InfoNES.o \
    $(BUILD_DIR)/InfoNES_Mapper.o \
    $(BUILD_DIR)/InfoNES_pAPU.o \
    $(BUILD_DIR)/K6502.o \
    $(BUILD_DIR)/InfoNES_System_SDL2.o

.PHONY: all clean

all: $(TARGET)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/InfoNES.o: $(INFONES_DIR)/InfoNES.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/InfoNES_Mapper.o: $(INFONES_DIR)/InfoNES_Mapper.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# pAPU has known issues at -O3 in older gcc; keep at -O0 to be safe.
$(BUILD_DIR)/InfoNES_pAPU.o: $(INFONES_DIR)/InfoNES_pAPU.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS:-O2=-O0) -c $< -o $@

$(BUILD_DIR)/K6502.o: $(INFONES_DIR)/K6502.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/InfoNES_System_SDL2.o: InfoNES_System_SDL2.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(TARGET): $(OBJS)
	$(CXX) $(OBJS) $(LDLIBS) -o $@

clean:
	rm -rf $(BUILD_DIR)
