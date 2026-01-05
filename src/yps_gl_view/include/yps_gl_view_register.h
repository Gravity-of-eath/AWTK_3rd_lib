/**
 * File:   yps_gl_view_register.h
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

#ifndef TK_YPS_GL_VIEW_REGISTER_H
#define TK_YPS_GL_VIEW_REGISTER_H

#include "base/types_def.h"

BEGIN_C_DECLS

/**
 * @method yps_gl_view_register
 * 注册控件
 * @annotation ["scriptable"]
 * @return {ret_t} 返回RET_OK表示成功，否则表示失败。
 */
ret_t yps_gl_view_register(void);

END_C_DECLS

#endif /*TK_YPS_GL_VIEW_REGISTER_H*/