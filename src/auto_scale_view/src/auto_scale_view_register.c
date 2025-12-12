/**
 * File:   auto_scale_view_register.c
 * Author: 云片松
 * Brief:  auto_scale_view register
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
 * 2025-11-18 created
 *
 */

#include "tkc/mem.h"
#include "tkc/utils.h"
#include "auto_scale_view_register.h"
#include "base/widget_factory.h"
#include "auto_scale_view.h"

ret_t auto_scale_view_register(void) {
  return widget_factory_register(widget_factory(), WIDGET_TYPE_AUTO_SCALE_VIEW, auto_scale_view_create);
}

const char* auto_scale_view_supported_render_mode(void) {
  return "OpenGL|AGGE-BGR565|AGGE-BGRA8888|AGGE-MONO";
}