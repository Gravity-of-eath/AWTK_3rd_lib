/**
 * File:   yps_cirle_gauge_register.h
 * Author: 云片松
 * Brief:  yps_cirle_gauge
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

#ifndef TK_AUTO_SCALE_VIEW_REGISTER_H
#define TK_AUTO_SCALE_VIEW_REGISTER_H

#include "base/widget.h"

BEGIN_C_DECLS

/**
 * @method  auto_scale_view_register
 * 注册控件。
 *
 * @annotation ["global"]
 *
 * @return {ret_t} 返回RET_OK表示成功，否则表示失败。
 */
ret_t auto_scale_view_register(void);

/**
 * @method  auto_scale_view_supported_render_mode
 * 获取支持的渲染模式。
 *
 * @annotation ["global"]
 *
 * @return {const char*} 返回渲染模式。
 */
const char* auto_scale_view_supported_render_mode(void);

END_C_DECLS

#endif /*TK_AUTO_SCALE_VIEW_REGISTER_H*/
