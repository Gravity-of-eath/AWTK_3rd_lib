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
