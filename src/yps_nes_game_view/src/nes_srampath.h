#ifndef YPS_NES_SRAMPATH_H
#define YPS_NES_SRAMPATH_H
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Build sram_dir + / + basename_no_ext(rom_path) + .srm.
 * Returns number of chars written on success (not counting NUL),
 * negative on overflow. */
int nes_srampath_build(const char* sram_dir, const char* rom_path,
                       char* out, size_t out_size);
#ifdef __cplusplus
}
#endif
#endif
