/**
 * File:   yps_gl_view_register.c
 * Author: 云片松
 * Brief:  注册 yps_gl_view 控件
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
 * 2025-11-12 created
 *
 */

#include "yps_gl_view.h"
#include "tkc/mem.h"

ret_t yps_gl_view_register(void) {
  return widget_register(WIDGET_TYPE_YPS_GL_VIEW, yps_gl_view_create);
}