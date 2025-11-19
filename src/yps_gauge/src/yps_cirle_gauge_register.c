/**
 * File:   yps_cirle_gauge_register.c
 * Author: 云片松
 * Brief:  yps_cirle_gauge register
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
#include "yps_cirle_gauge_register.h"
#include "base/widget_factory.h"
#include "yps_cirle_gauge.h"

ret_t yps_cirle_gauge_register(void) {
  return widget_factory_register(widget_factory(), WIDGET_TYPE_YPS_CIRLE_GAUGE, yps_cirle_gauge_create);
}

const char* yps_cirle_gauge_supported_render_mode(void) {
  return "OpenGL|AGGE-BGR565|AGGE-BGRA8888|AGGE-MONO";
}