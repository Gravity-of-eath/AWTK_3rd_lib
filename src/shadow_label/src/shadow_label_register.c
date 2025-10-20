/**
 * File:   shadow_label.c
 * Author: 
 * Brief:  
 *
 * Copyright (c) 2025 - 2025 
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * License file for more details.
 *
 */

/**
 * History:
 * ================================================================
 * 2025-2-21  created
 *
 */


#include "tkc/mem.h"
#include "tkc/utils.h"
#include "shadow_label_register.h"
#include "base/widget_factory.h"
#include "shadow_label.h"

ret_t shadow_label_register(void) {
  return widget_factory_register(widget_factory(), WIDGET_TYPE_SHADOW_LABEL, shadow_label_create);
}

const char* shadow_label_supported_render_mode(void) {
  return "OpenGL|AGGE-BGR565|AGGE-BGRA8888|AGGE-MONO";
}
