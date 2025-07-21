/**
 * File:   yps_banner_menu.c
 * Author: yps
 * Brief:  yps_banner_menu
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
 * 2025-6-3 yk created
 *
 */


#include "tkc/mem.h"
#include "tkc/utils.h"
#include "yps_banner_menu_register.h"
#include "base/widget_factory.h"
#include "yps_banner_menu.h"

ret_t yps_banner_menu_register(void) {
  return widget_factory_register(widget_factory(), WIDGET_TYPE_YPS_BANNER_MENU, yps_banner_menu_create);
}

const char* yps_banner_menu_supported_render_mode(void) {
  return "OpenGL|AGGE-BGR565|AGGE-BGRA8888|AGGE-MONO";
}
