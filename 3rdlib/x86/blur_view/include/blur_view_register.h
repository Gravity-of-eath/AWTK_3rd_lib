/**
 * File:   blur_view_register.h
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


#ifndef TK_LINE_CHART_REGISTER_H
#define TK_LINE_CHART_REGISTER_H

#include "base/widget.h"

BEGIN_C_DECLS

/**
 * @method  blur_view_register
 * 注册控件。
 *
 * @annotation ["global"]
 *
 * @return {ret_t} 返回RET_OK表示成功，否则表示失败。
 */
ret_t blur_view_register(void);

/**
 * @method  blur_view_supported_render_mode
 * 获取支持的渲染模式。
 *
 * @annotation ["global"]
 *
 * @return {const char*} 返回渲染模式。
 */
const char* blur_view_supported_render_mode(void);

END_C_DECLS

#endif /*TK_LINE_CHART_REGISTER_H*/
