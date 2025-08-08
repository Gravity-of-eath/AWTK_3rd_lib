/**
 * File:   blur_view.c
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
 * 2025-7-17  created
 *
 */

#include "tkc/mem.h"
#include "tkc/utils.h"
#include "blur_view_register.h"
#include "base/widget_factory.h"
#include "blur_view.h"
#include "view_ext.h"

ret_t blur_view_register(void)
{
  widget_factory_register(widget_factory(), WIDGET_TYPE_VIEW_EXT, view_ext_create);
  return widget_factory_register(widget_factory(), WIDGET_TYPE_BLUR_VIEW, blur_view_create);
}

const char *blur_view_supported_render_mode(void)
{
  return "OpenGL|AGGE-BGR565|AGGE-BGRA8888|AGGE-MONO";
}
