/**
 * File:   view_ext.h
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

#ifndef TK_VIEW_EXT_H
#define TK_VIEW_EXT_H

#include "base/widget.h"

BEGIN_C_DECLS
/**
 * @class view_ext_t
 * @parent widget_t
 * @annotation ["scriptable","design","widget"]
 *
 * 在xml中使用"line\_chart"标签创建控件。如：
 *
 * ```xml
 * <!-- ui -->
 * <view_ext x="c" y="50" w="100" h="100"/>
 * ```
 *
 * 可用通过style来设置控件的显示风格，如字体的大小和颜色等等。如：
 *
 * ```xml
 * <!-- style -->
 * <view_ext>
 *   <style name="default" font_size="32">
 *     <normal text_color="black" />
 *   </style>
 * </view_ext>
 * ```
 */
typedef struct _view_ext_t
{
  widget_t widget;
  widget_t *abort_widget;

} view_ext_t;

void view_ext_set_abort_widget(widget_t *widget,widget_t *abort_widget);

widget_t *view_ext_create(widget_t *parent, xy_t x, xy_t y, wh_t w, wh_t h);
 
widget_t *view_ext_cast(widget_t *widget);

#define WIDGET_TYPE_VIEW_EXT "view_ext"

#define VIEW_EXT(widget) ((view_ext_t *)(view_ext_cast(WIDGET(widget))))

/*public for subclass and runtime type check*/
TK_EXTERN_VTABLE(view_ext);

END_C_DECLS

#endif /*TK_VIEW_EXT_H*/
