/**
 * File:   line_chart.c
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
#include "line_chart_register.h"
#include "base/widget_factory.h"
#include "line_chart.h"

ret_t line_chart_register(void) {
  return widget_factory_register(widget_factory(), WIDGET_TYPE_LINE_CHART, line_chart_create);
}

const char* line_chart_supported_render_mode(void) {
  return "OpenGL|AGGE-BGR565|AGGE-BGRA8888|AGGE-MONO";
}
