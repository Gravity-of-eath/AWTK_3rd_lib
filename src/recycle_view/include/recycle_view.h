/**
 * File:   recycle_view.h
 * Brief:  类 Android RecyclerView 的可复用视图容器
 */
#ifndef TK_RECYCLE_VIEW_H
#define TK_RECYCLE_VIEW_H

#include "base/widget.h"
#include "tkc/darray.h"
#include "base/velocity.h"
#include "recycle_adapter.h"
#include "recycle_layout_manager.h"

BEGIN_C_DECLS

/**
 * @class recycle_view_t
 * @parent widget_t
 * @annotation ["scriptable","design","widget"]
 * 类 Android RecyclerView 的可复用视图容器。通过 adapter + layout_manager 驱动，
 * 滚出可视区域的 item 控件回收进池并复用。
 *
 * 在 xml 中使用 "recycle_view" 标签创建控件。如：
 *
 * ```xml
 * <recycle_view x="0" y="0" w="240" h="320"/>
 * ```
 * adapter 与 layout_manager 通过 C API 设置。
 */
typedef struct _recycle_view_t {
  widget_t widget;

  /* 私有变量，不要直接访问 */
  recycle_adapter_t* adapter;
  recycle_layout_manager_t* layout_manager;

  int32_t xoffset;
  int32_t yoffset;
  int32_t item_count;

  darray_t* visible_items; /* 元素类型 visible_item_t* */
  darray_t* recycle_pools; /* 元素类型 recycle_pool_t* */

  velocity_t velocity;
  float_t fling_v;
  uint32_t fling_timer_id;
  bool_t dragging;
  xy_t down_x;
  xy_t down_y;
  int32_t down_offset;

  uint32_t scroll_animator_id;
  int32_t scroll_target;   /* 动画目标 offset */
  uint32_t scroll_timer_id;
} recycle_view_t;

widget_t* recycle_view_create(widget_t* parent, xy_t x, xy_t y, wh_t w, wh_t h);
recycle_view_t* recycle_view_cast(widget_t* widget);

/* set_* 后 recycle_view 接管所有权，销毁时调用对应 on_destroy 释放 */
ret_t recycle_view_set_adapter(widget_t* widget, recycle_adapter_t* adapter);
ret_t recycle_view_set_layout_manager(widget_t* widget, recycle_layout_manager_t* lm);

ret_t recycle_view_scroll_to_index(widget_t* widget, int32_t index, bool_t animate);
ret_t recycle_view_scroll_to_offset(widget_t* widget, int32_t offset, bool_t animate);
ret_t recycle_view_notify_data_changed(widget_t* widget);

#define WIDGET_TYPE_RECYCLE_VIEW "recycle_view"

#define RECYCLE_VIEW(widget) ((recycle_view_t*)(recycle_view_cast(WIDGET(widget))))

TK_EXTERN_VTABLE(recycle_view);

END_C_DECLS

#endif /*TK_RECYCLE_VIEW_H*/
