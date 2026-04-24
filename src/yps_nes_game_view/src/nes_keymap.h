#ifndef YPS_NES_KEYMAP_H
#define YPS_NES_KEYMAP_H
#include <stdint.h>
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif

/**
 * Slot indices for `defaults` / `out` arrays:
 * 0=UP 1=DOWN 2=LEFT 3=RIGHT 4=A 5=B 6=START 7=SELECT
 *
 * map_str syntax: "name=KEY,name=KEY,..." where name in {up,down,left,right,a,b,start,select}
 * and KEY is a TK_KEY_* symbolic name (without the TK_KEY_ prefix), case-insensitive.
 * Single-char keys like X, Z, A are parsed as TK_KEY_<lower>, e.g. "X" -> TK_KEY_x.
 *
 * Unspecified slots keep defaults. Unknown slot names silently ignored.
 * Returns number of slots successfully overridden.
 */
int nes_keymap_parse(const char* map_str, const uint32_t defaults[8], uint32_t out[8]);

/* key name -> TK_KEY_* code; 0 if unknown. Public for tests. */
uint32_t nes_keymap_name_to_code(const char* name, size_t len);

#ifdef __cplusplus
}
#endif
#endif
