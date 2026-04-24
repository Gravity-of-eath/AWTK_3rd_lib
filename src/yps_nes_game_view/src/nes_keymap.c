#include "nes_keymap.h"
#include "base/keys.h"
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

static int ieq(const char* a, size_t a_len, const char* b) {
  size_t b_len = strlen(b);
  if (a_len != b_len) return 0;
  for (size_t i = 0; i < a_len; i++) {
    if (tolower((unsigned char)a[i]) != tolower((unsigned char)b[i])) return 0;
  }
  return 1;
}

/* Slot name -> slot index (0..7); -1 if unknown */
static int slot_index(const char* name, size_t len) {
  if (ieq(name, len, "up"))     return 0;
  if (ieq(name, len, "down"))   return 1;
  if (ieq(name, len, "left"))   return 2;
  if (ieq(name, len, "right"))  return 3;
  if (ieq(name, len, "a"))      return 4;
  if (ieq(name, len, "b"))      return 5;
  if (ieq(name, len, "start"))  return 6;
  if (ieq(name, len, "select")) return 7;
  return -1;
}

uint32_t nes_keymap_name_to_code(const char* name, size_t len) {
  /* Named symbolic keys */
  if (ieq(name, len, "UP"))       return TK_KEY_UP;
  if (ieq(name, len, "DOWN"))     return TK_KEY_DOWN;
  if (ieq(name, len, "LEFT"))     return TK_KEY_LEFT;
  if (ieq(name, len, "RIGHT"))    return TK_KEY_RIGHT;
  if (ieq(name, len, "RETURN"))   return TK_KEY_RETURN;
  if (ieq(name, len, "ENTER"))    return TK_KEY_RETURN;
  if (ieq(name, len, "ESCAPE"))   return TK_KEY_ESCAPE;
  if (ieq(name, len, "ESC"))      return TK_KEY_ESCAPE;
  if (ieq(name, len, "SPACE"))    return TK_KEY_SPACE;
  if (ieq(name, len, "TAB"))      return TK_KEY_TAB;
  if (ieq(name, len, "BACKSPACE"))return TK_KEY_BACKSPACE;
  /* Single ASCII char -> lowercase key code (TK_KEY_a..z and digits) */
  if (len == 1) {
    char c = (char)tolower((unsigned char)name[0]);
    return (uint32_t)c;  /* TK_KEY_a..z / '0'..'9' are ASCII values per awtk */
  }
  return 0;
}

/* Trim leading/trailing whitespace; updates *p and *len */
static void trim(const char** p, size_t* len) {
  while (*len > 0 && isspace((unsigned char)(**p))) { (*p)++; (*len)--; }
  while (*len > 0 && isspace((unsigned char)((*p)[*len - 1]))) { (*len)--; }
}

int nes_keymap_parse(const char* map_str, const uint32_t defaults[8], uint32_t out[8]) {
  for (int i = 0; i < 8; i++) out[i] = defaults[i];
  if (map_str == NULL || *map_str == '\0') return 0;

  int count = 0;
  const char* p = map_str;

  while (*p) {
    /* Skip separators */
    while (*p == ',' || *p == ';' || isspace((unsigned char)*p)) p++;
    if (!*p) break;

    /* Find '=' */
    const char* eq = strchr(p, '=');
    if (!eq) break;

    const char* name = p;
    size_t name_len = (size_t)(eq - p);
    trim(&name, &name_len);

    const char* val = eq + 1;
    const char* end = val;
    while (*end && *end != ',' && *end != ';') end++;
    size_t val_len = (size_t)(end - val);
    trim(&val, &val_len);

    int slot = slot_index(name, name_len);
    if (slot >= 0 && val_len > 0) {
      uint32_t code = nes_keymap_name_to_code(val, val_len);
      if (code != 0) {
        out[slot] = code;
        count++;
      }
    }
    p = (*end) ? end + 1 : end;
  }
  return count;
}
