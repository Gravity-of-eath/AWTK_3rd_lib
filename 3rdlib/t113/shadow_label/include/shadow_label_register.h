/**
 * File:   shadow_label_register.h
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


#ifndef TK_SHADOW_LABEL_REGISTER_H
#define TK_SHADOW_LABEL_REGISTER_H

#include "base/widget.h"

BEGIN_C_DECLS

/**
 * @method  shadow_label_register
 * 注册控件。
 *
 * @annotation ["global"]
 *
 * @return {ret_t} 返回RET_OK表示成功，否则表示失败。
 */
ret_t shadow_label_register(void);

/**
 * @method  shadow_label_supported_render_mode
 * 获取支持的渲染模式。
 *
 * @annotation ["global"]
 *
 * @return {const char*} 返回渲染模式。
 */
const char* shadow_label_supported_render_mode(void);

END_C_DECLS

#endif /*TK_SHADOW_LABEL_REGISTER_H*/
