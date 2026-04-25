/*===================================================================*/
/*                                                                   */
/*  InfoNES_System_SDL2.cpp : minimal SDL2-based system layer        */
/*                                                                   */
/*  A standalone NES emulator based on InfoNES, used as a reference  */
/*  implementation to sanity-check the core before debugging the     */
/*  AWTK widget integration.                                         */
/*                                                                   */
/*  Video: SDL_Renderer + streaming RGB565 texture                   */
/*  Audio: disabled (silent smoke build)                             */
/*  Input: keyboard only (Player 1)                                  */
/*                                                                   */
/*===================================================================*/

#include <SDL.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdarg>

#include "../../InfoNES/src/InfoNES.h"
#include "../../InfoNES/src/InfoNES_System.h"
#include "../../InfoNES/src/InfoNES_pAPU.h"

/*-------------------------------------------------------------------*/
/*  Global palette (declared extern in InfoNES_System.h).            */
/*-------------------------------------------------------------------*/
WORD NesPalette[64] = {0};

/* NES base palette (RGB triplets, 0-255). */
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

static inline uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b) {
  return (uint16_t)(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
}

/*-------------------------------------------------------------------*/
/*  SDL2 state + input                                               */
/*-------------------------------------------------------------------*/
static SDL_Window*   g_window   = nullptr;
static SDL_Renderer* g_renderer = nullptr;
static SDL_Texture*  g_tex      = nullptr;

static DWORD g_pad1     = 0;
static DWORD g_sys_req  = 0;
static bool  g_quit     = false;

/* Key → InfoNES pad bit (SDL-port convention):
 *   0:A 1:B 2:SELECT 3:START 4:UP 5:DOWN 6:LEFT 7:RIGHT */
static bool key_to_pad_bit(SDL_Keycode k, int* bit) {
  switch (k) {
    case SDLK_x:      *bit = 0; return true;   /* A */
    case SDLK_z:      *bit = 1; return true;   /* B */
    case SDLK_SPACE:  *bit = 2; return true;   /* SELECT */
    case SDLK_RETURN: *bit = 3; return true;   /* START */
    case SDLK_UP:     *bit = 4; return true;
    case SDLK_DOWN:   *bit = 5; return true;
    case SDLK_LEFT:   *bit = 6; return true;
    case SDLK_RIGHT:  *bit = 7; return true;
    default: return false;
  }
}

static void pump_events(void) {
  SDL_Event e;
  while (SDL_PollEvent(&e)) {
    switch (e.type) {
      case SDL_QUIT:
        g_quit = true;
        break;
      case SDL_KEYDOWN:
      case SDL_KEYUP: {
        if (e.key.keysym.sym == SDLK_ESCAPE && e.type == SDL_KEYDOWN) {
          g_quit = true;
          break;
        }
        int bit;
        if (key_to_pad_bit(e.key.keysym.sym, &bit)) {
          DWORD mask = (DWORD)(1u << bit);
          if (e.type == SDL_KEYDOWN) g_pad1 |=  mask;
          else                       g_pad1 &= ~mask;
        }
        break;
      }
      default: break;
    }
  }
}

/*-------------------------------------------------------------------*/
/*  InfoNES_System callbacks                                         */
/*-------------------------------------------------------------------*/

int InfoNES_Menu(void) {
  return g_quit ? -1 : 0;
}

/* Cooperative-stop hook used by InfoNES_HSync when compiled with
 * -DINFONES_AWTK_GLUE. Lets SIGTERM / SDL_QUIT / Esc break out of the
 * Cycle() loop within a scanline rather than waiting for InfoNES_Menu. */
extern "C" int infones_glue_should_stop(void) {
  return g_quit ? 1 : 0;
}

int InfoNES_ReadRom(const char* pszFileName) {
  FILE* fp = fopen(pszFileName, "rb");
  if (!fp) return -1;
  if (fread(&NesHeader, sizeof NesHeader, 1, fp) != 1 ||
      memcmp(NesHeader.byID, "NES\x1a", 4) != 0) {
    fclose(fp);
    return -1;
  }
  memset(SRAM, 0, SRAM_SIZE);
  if (NesHeader.byInfo1 & 4) {
    if (fread(&SRAM[0x1000], 512, 1, fp) != 1) { fclose(fp); return -1; }
  }
  ROM = (BYTE*)malloc((size_t)NesHeader.byRomSize * 0x4000);
  if (!ROM) { fclose(fp); return -1; }
  if (fread(ROM, 0x4000, NesHeader.byRomSize, fp) != NesHeader.byRomSize) {
    free(ROM); ROM = nullptr; fclose(fp); return -1;
  }
  if (NesHeader.byVRomSize > 0) {
    VROM = (BYTE*)malloc((size_t)NesHeader.byVRomSize * 0x2000);
    if (!VROM) { free(ROM); ROM = nullptr; fclose(fp); return -1; }
    if (fread(VROM, 0x2000, NesHeader.byVRomSize, fp) != NesHeader.byVRomSize) {
      free(ROM); ROM = nullptr; free(VROM); VROM = nullptr; fclose(fp); return -1;
    }
  }
  fclose(fp);
  return 0;
}

void InfoNES_ReleaseRom(void) {
  if (ROM)  { free(ROM);  ROM = nullptr; }
  if (VROM) { free(VROM); VROM = nullptr; }
}

void InfoNES_LoadFrame(void) {
  pump_events();

  /* Upload WorkFrame (256x240 RGB565) into the streaming texture. */
  void* pixels = nullptr;
  int pitch = 0;
  if (SDL_LockTexture(g_tex, nullptr, &pixels, &pitch) == 0) {
    const int row_bytes = NES_DISP_WIDTH * (int)sizeof(WORD);
    if (pitch == row_bytes) {
      memcpy(pixels, WorkFrame, (size_t)row_bytes * NES_DISP_HEIGHT);
    } else {
      uint8_t* dst = (uint8_t*)pixels;
      const uint8_t* src = (const uint8_t*)WorkFrame;
      for (int y = 0; y < NES_DISP_HEIGHT; y++) {
        memcpy(dst + (size_t)y * pitch, src + (size_t)y * row_bytes, row_bytes);
      }
    }
    SDL_UnlockTexture(g_tex);
  }

  SDL_RenderClear(g_renderer);
  SDL_RenderCopy(g_renderer, g_tex, nullptr, nullptr);
  SDL_RenderPresent(g_renderer);
}

void InfoNES_PadState(DWORD* pdwPad1, DWORD* pdwPad2, DWORD* pdwSystem) {
  *pdwPad1   = g_pad1;
  *pdwPad2   = 0;
  *pdwSystem = g_sys_req;
}

void* InfoNES_MemoryCopy(void* dest, const void* src, int count) {
  return memcpy(dest, src, (size_t)count);
}
void* InfoNES_MemorySet(void* dest, int c, int count) {
  return memset(dest, c, (size_t)count);
}
void InfoNES_DebugPrint(char* pszMsg) {
  fprintf(stderr, "%s\n", pszMsg);
}

void InfoNES_Wait(void) {
  /* Coarse 60Hz pacing: InfoNES_Wait is called every scanline (~262/frame).
   * Once per frame, sleep to the next 1/60s boundary. */
  static int scanline = 0;
  static Uint32 last_ticks = 0;
  if (++scanline < 262) return;
  scanline = 0;
  Uint32 now = SDL_GetTicks();
  if (last_ticks != 0) {
    Uint32 target = last_ticks + 16;  /* ~60fps */
    if (now < target) SDL_Delay(target - now);
  }
  last_ticks = SDL_GetTicks();
}

/* Silent build: audio callbacks do nothing. */
void InfoNES_SoundInit(void) {}
int  InfoNES_SoundOpen(int, int) { return 0; }
void InfoNES_SoundClose(void) {}
void InfoNES_SoundOutput(int, BYTE*, BYTE*, BYTE*, BYTE*, BYTE*) {}

void InfoNES_MessageBox(char* fmt, ...) {
  va_list ap; va_start(ap, fmt);
  fprintf(stderr, "[InfoNES] ");
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  va_end(ap);
}

/*-------------------------------------------------------------------*/
/*  main                                                             */
/*-------------------------------------------------------------------*/

static int init_sdl(int scale) {
  if (SDL_Init(SDL_INIT_VIDEO) != 0) {
    fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
    return -1;
  }

  g_window = SDL_CreateWindow(
      "InfoNES (SDL2)",
      SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
      NES_DISP_WIDTH * scale, NES_DISP_HEIGHT * scale,
      SDL_WINDOW_SHOWN);
  if (!g_window) { fprintf(stderr, "CreateWindow: %s\n", SDL_GetError()); return -1; }

  g_renderer = SDL_CreateRenderer(g_window, -1,
      SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  if (!g_renderer) { fprintf(stderr, "CreateRenderer: %s\n", SDL_GetError()); return -1; }

  SDL_RenderSetLogicalSize(g_renderer, NES_DISP_WIDTH, NES_DISP_HEIGHT);

  g_tex = SDL_CreateTexture(g_renderer,
      SDL_PIXELFORMAT_RGB565, SDL_TEXTUREACCESS_STREAMING,
      NES_DISP_WIDTH, NES_DISP_HEIGHT);
  if (!g_tex) { fprintf(stderr, "CreateTexture: %s\n", SDL_GetError()); return -1; }
  return 0;
}

static void shutdown_sdl(void) {
  if (g_tex)      SDL_DestroyTexture(g_tex);
  if (g_renderer) SDL_DestroyRenderer(g_renderer);
  if (g_window)   SDL_DestroyWindow(g_window);
  SDL_Quit();
}

int main(int argc, char** argv) {
  if (argc < 2) {
    fprintf(stderr, "usage: %s <rom.nes> [scale]\n", argv[0]);
    return 1;
  }
  int scale = (argc >= 3) ? atoi(argv[2]) : 2;
  if (scale < 1) scale = 1;

  if (init_sdl(scale) != 0) { shutdown_sdl(); return 2; }

  /* Populate palette (RGB565). */
  for (int i = 0; i < 64; i++) {
    NesPalette[i] = rgb565(kNesPaletteRGB[i][0],
                           kNesPaletteRGB[i][1],
                           kNesPaletteRGB[i][2]);
  }

  if (InfoNES_Load(argv[1]) != 0) {
    fprintf(stderr, "Failed to load ROM: %s\n", argv[1]);
    shutdown_sdl();
    return 3;
  }

  printf("InfoNES running. Keys: arrows=D-pad  X=A  Z=B  Space=Select  Enter=Start  Esc=quit\n");
  InfoNES_Main();

  shutdown_sdl();
  return 0;
}
