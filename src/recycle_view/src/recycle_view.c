#include "recycle_view.h"

#include "tkc/mem.h"
#include "tkc/utils.h"
#include "base/timer.h"
#include "base/widget_vtable.h"
#include "recycle_view_math.h"

/* 当前挂载的可见项 */
typedef struct _visible_item_t {
  int32_t index;
  int32_t view_type;
  widget_t* widget;
} visible_item_t;

/* 某 view_type 的回收池 */
typedef struct _recycle_pool_t {
  int32_t view_type;
  darray_t* free_widgets; /* 元素 widget_t*（detach 状态，未销毁） */
} recycle_pool_t;

/* 预取冗余：可见区间前后各多保留的项数，减少快速滑动时的临界抖动 */
#define RECYCLE_VIEW_PREFETCH 1
#define RECYCLE_VIEW_FRAME_INTERVAL_MS 16
#define RECYCLE_VIEW_FLING_FRICTION 0.92f
#define RECYCLE_VIEW_FLING_MIN_V 0.5f
#define RECYCLE_VIEW_DRAG_THRESHOLD 3

/* 后续任务实现，先前向声明，供 setter 调用 */
static ret_t recycle_view_relayout(widget_t* widget);

/* ---- 回收池辅助 ---- */

static recycle_pool_t* recycle_view_get_pool(recycle_view_t* rv, int32_t view_type, bool_t create) {
  uint32_t i = 0;
  recycle_pool_t* pool = NULL;
  for (i = 0; i < rv->recycle_pools->size; i++) {
    pool = (recycle_pool_t*)darray_get(rv->recycle_pools, i);
    if (pool != NULL && pool->view_type == view_type) {
      return pool;
    }
  }
  if (!create) {
    return NULL;
  }
  pool = TKMEM_ZALLOC(recycle_pool_t);
  return_value_if_fail(pool != NULL, NULL);
  pool->view_type = view_type;
  pool->free_widgets = darray_create(8, NULL, NULL); /* 不在此销毁 widget */
  darray_push(rv->recycle_pools, pool);
  return pool;
}

static ret_t recycle_pool_destroy(void* data) {
  recycle_pool_t* pool = (recycle_pool_t*)data;
  if (pool != NULL) {
    /* 池中是 detach 状态的 widget，需在此销毁 */
    uint32_t i = 0;
    for (i = 0; i < pool->free_widgets->size; i++) {
      widget_t* w = (widget_t*)darray_get(pool->free_widgets, i);
      if (w != NULL) {
        widget_destroy(w);
      }
    }
    darray_destroy(pool->free_widgets);
    TKMEM_FREE(pool);
  }
  return RET_OK;
}

/* ---- ownership setters ---- */

ret_t recycle_view_set_adapter(widget_t* widget, recycle_adapter_t* adapter) {
  recycle_view_t* rv = RECYCLE_VIEW(widget);
  return_value_if_fail(rv != NULL && adapter != NULL, RET_BAD_PARAMS);
  if (rv->adapter != NULL && rv->adapter->on_destroy != NULL) {
    rv->adapter->on_destroy(rv->adapter);
  }
  rv->adapter = adapter;
  rv->yoffset = 0;
  rv->xoffset = 0;
  recycle_view_relayout(widget);
  return RET_OK;
}

ret_t recycle_view_set_layout_manager(widget_t* widget, recycle_layout_manager_t* lm) {
  recycle_view_t* rv = RECYCLE_VIEW(widget);
  return_value_if_fail(rv != NULL && lm != NULL, RET_BAD_PARAMS);
  if (rv->layout_manager != NULL && rv->layout_manager->on_destroy != NULL) {
    rv->layout_manager->on_destroy(rv->layout_manager);
  }
  rv->layout_manager = lm;
  recycle_view_relayout(widget);
  return RET_OK;
}

/* ---- relayout 占位（后续任务替换实现） ---- */
static ret_t recycle_view_relayout(widget_t* widget) {
  (void)widget;
  return RET_OK;
}

/* ---- scroll_to / notify 占位（后续任务替换实现） ---- */
ret_t recycle_view_scroll_to_offset(widget_t* widget, int32_t offset, bool_t animate) {
  (void)widget;
  (void)offset;
  (void)animate;
  return RET_OK;
}

ret_t recycle_view_scroll_to_index(widget_t* widget, int32_t index, bool_t animate) {
  (void)widget;
  (void)index;
  (void)animate;
  return RET_OK;
}

ret_t recycle_view_notify_data_changed(widget_t* widget) {
  return recycle_view_relayout(widget);
}

/* ---- vtable callbacks ---- */

static ret_t recycle_view_on_destroy(widget_t* widget) {
  recycle_view_t* rv = RECYCLE_VIEW(widget);
  return_value_if_fail(rv != NULL, RET_BAD_PARAMS);

  if (rv->fling_timer_id != 0) {
    timer_remove(rv->fling_timer_id);
    rv->fling_timer_id = 0;
  }
  if (rv->visible_items != NULL) {
    darray_destroy(rv->visible_items);
    rv->visible_items = NULL;
  }
  if (rv->recycle_pools != NULL) {
    darray_destroy(rv->recycle_pools); /* recycle_pool_destroy 会销毁池中 detach 的 widget */
    rv->recycle_pools = NULL;
  }
  if (rv->layout_manager != NULL && rv->layout_manager->on_destroy != NULL) {
    rv->layout_manager->on_destroy(rv->layout_manager);
    rv->layout_manager = NULL;
  }
  if (rv->adapter != NULL && rv->adapter->on_destroy != NULL) {
    rv->adapter->on_destroy(rv->adapter);
    rv->adapter = NULL;
  }
  return RET_OK;
}

static ret_t recycle_view_on_event(widget_t* widget, event_t* e) {
  (void)widget;
  (void)e;
  return RET_OK; /* 后续任务填充拖动与 fling */
}

TK_DECL_VTABLE(recycle_view) = {.size = sizeof(recycle_view_t),
                                .type = WIDGET_TYPE_RECYCLE_VIEW,
                                .inputable = TRUE,
                                .parent = TK_PARENT_VTABLE(widget),
                                .create = recycle_view_create,
                                .on_paint_children = widget_on_paint_children_clip,
                                .on_event = recycle_view_on_event,
                                .on_destroy = recycle_view_on_destroy};

widget_t* recycle_view_create(widget_t* parent, xy_t x, xy_t y, wh_t w, wh_t h) {
  widget_t* widget = widget_create(parent, TK_REF_VTABLE(recycle_view), x, y, w, h);
  recycle_view_t* rv = RECYCLE_VIEW(widget);
  return_value_if_fail(rv != NULL, NULL);

  rv->adapter = NULL;
  rv->layout_manager = NULL;
  rv->xoffset = 0;
  rv->yoffset = 0;
  rv->item_count = 0;
  rv->visible_items = darray_create(16, NULL, NULL);
  rv->recycle_pools = darray_create(4, recycle_pool_destroy, NULL);
  rv->fling_v = 0.0f;
  rv->fling_timer_id = 0;
  rv->dragging = FALSE;
  rv->scroll_animator_id = 0;
  velocity_reset(&(rv->velocity));

  return widget;
}

recycle_view_t* recycle_view_cast(widget_t* widget) {
  return_value_if_fail(WIDGET_IS_INSTANCE_OF(widget, recycle_view), NULL);
  return (recycle_view_t*)widget;
}
