#include "recycle_view.h"

#include <math.h>
#include "tkc/mem.h"
#include "tkc/utils.h"
#include "tkc/log.h"
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

static ret_t visible_item_destroy(void* data) {
  /* 仅释放裸结构；widget 的归属由回收/子控件树管理，不在此销毁 */
  if (data != NULL) {
    TKMEM_FREE(data);
  }
  return RET_OK;
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

/* 清空所有可见项与回收池中的控件（用于切换 adapter/lm 时彻底重建） */
static ret_t recycle_view_clear_all_items(recycle_view_t* rv) {
  uint32_t k = 0;
  return_value_if_fail(rv != NULL, RET_BAD_PARAMS);
  /* 可见项：detach + 销毁控件，再清空跟踪结构 */
  for (k = 0; k < rv->visible_items->size; k++) {
    visible_item_t* vi = (visible_item_t*)darray_get(rv->visible_items, k);
    if (vi != NULL && vi->widget != NULL) {
      widget_remove_child((widget_t*)rv, vi->widget);
      widget_destroy(vi->widget);
    }
  }
  darray_clear(rv->visible_items); /* 触发 visible_item_destroy 释放裸结构 */
  /* 回收池：recycle_pool_destroy 会销毁池内 detach 的控件 */
  darray_clear(rv->recycle_pools);
  return RET_OK;
}

ret_t recycle_view_set_adapter(widget_t* widget, recycle_adapter_t* adapter) {
  recycle_view_t* rv = RECYCLE_VIEW(widget);
  return_value_if_fail(rv != NULL && adapter != NULL, RET_BAD_PARAMS);
  if (rv->adapter != NULL) {
    recycle_view_clear_all_items(rv); /* 用旧 adapter 上下文清空旧控件 */
    if (rv->adapter->on_destroy != NULL) {
      rv->adapter->on_destroy(rv->adapter);
    }
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

static visible_item_t* recycle_view_find_visible(recycle_view_t* rv, int32_t index) {
  uint32_t i = 0;
  for (i = 0; i < rv->visible_items->size; i++) {
    visible_item_t* vi = (visible_item_t*)darray_get(rv->visible_items, i);
    if (vi != NULL && vi->index == index) {
      return vi;
    }
  }
  return NULL;
}

/* 取得某 view_type 的一个空壳：优先复用池，池空则新建 */
static widget_t* recycle_view_obtain_item(recycle_view_t* rv, int32_t view_type) {
  recycle_pool_t* pool = recycle_view_get_pool(rv, view_type, FALSE);
  if (pool != NULL && pool->free_widgets->size > 0) {
    widget_t* w = (widget_t*)darray_get(pool->free_widgets, pool->free_widgets->size - 1);
    darray_remove_index(pool->free_widgets, pool->free_widgets->size - 1);
    return w;
  }
  return rv->adapter->create_item_view(rv->adapter, (widget_t*)rv, view_type);
}

static int32_t recycle_view_main_offset(recycle_view_t* rv) {
  return rv->layout_manager->is_horizontal ? rv->xoffset : rv->yoffset;
}

static void recycle_view_set_main_offset(recycle_view_t* rv, int32_t offset) {
  if (rv->layout_manager->is_horizontal) {
    rv->xoffset = offset;
    rv->yoffset = 0;
  } else {
    rv->yoffset = offset;
    rv->xoffset = 0;
  }
}

static ret_t recycle_view_relayout(widget_t* widget) {
  recycle_view_t* rv = RECYCLE_VIEW(widget);
  recycle_layout_manager_t* lm = NULL;
  recycle_adapter_t* adapter = NULL;
  int32_t first = 0, last = -1, i = 0, offset = 0, content_size = 0, viewport_main = 0;
  return_value_if_fail(rv != NULL, RET_BAD_PARAMS);

  lm = rv->layout_manager;
  adapter = rv->adapter;
  if (lm == NULL || adapter == NULL) {
    return RET_OK; /* 未就绪：空白，不崩 */
  }

  rv->item_count = adapter->get_item_count != NULL ? adapter->get_item_count(adapter) : 0;

  /* clamp offset 到合法范围 */
  content_size = lm->get_content_size(lm, widget, rv->item_count);
  viewport_main = lm->is_horizontal ? widget->w : widget->h;
  offset = recycle_clamp_offset(recycle_view_main_offset(rv), content_size, viewport_main);
  recycle_view_set_main_offset(rv, offset);

  /* 计算可见区间并加预取冗余 */
  if (rv->item_count > 0) {
    lm->get_visible_range(lm, widget, offset, rv->item_count, &first, &last);
    first -= RECYCLE_VIEW_PREFETCH;
    last += RECYCLE_VIEW_PREFETCH;
    if (first < 0) first = 0;
    if (last > rv->item_count - 1) last = rv->item_count - 1;
  } else {
    first = 0;
    last = -1;
  }

  /* 回收：可见表中落在区间外的项 */
  {
    uint32_t k = 0;
    while (k < rv->visible_items->size) {
      visible_item_t* vi = (visible_item_t*)darray_get(rv->visible_items, k);
      if (vi != NULL && (vi->index < first || vi->index > last)) {
        recycle_pool_t* pool = NULL;
        widget_remove_child(widget, vi->widget);
        if (adapter->on_item_recycled != NULL) {
          adapter->on_item_recycled(adapter, vi->widget, vi->view_type);
        }
        pool = recycle_view_get_pool(rv, vi->view_type, TRUE);
        if (pool != NULL) {
          darray_push(pool->free_widgets, vi->widget);
        } else {
          widget_destroy(vi->widget);
        }
        darray_remove_index(rv->visible_items, k); /* visible_item_destroy 释放裸结构 */
      } else {
        k++;
      }
    }
  }

  /* 填充：区间内尚未挂载的 index */
  for (i = first; i <= last; i++) {
    rect_t r;
    int32_t view_type = 0;
    widget_t* item = NULL;
    visible_item_t* vi = NULL;
    if (recycle_view_find_visible(rv, i) != NULL) {
      continue;
    }
    view_type = (adapter->get_item_type != NULL) ? adapter->get_item_type(adapter, i) : 0;
    item = recycle_view_obtain_item(rv, view_type);
    if (item == NULL) {
      log_debug("recycle_view: create_item_view 返回 NULL，跳过 index=%d\n", i);
      continue;
    }
    adapter->bind_item_view(adapter, item, i);
    vi = TKMEM_ZALLOC(visible_item_t);
    if (vi == NULL) {
      /* 跟踪结构分配失败：销毁这个壳，避免遗留未跟踪子控件 */
      widget_destroy(item);
      continue;
    }
    widget_add_child(widget, item);
    lm->get_item_rect(lm, widget, i, &r);
    if (lm->is_horizontal) {
      widget_move_resize(item, r.x - offset, r.y, r.w, r.h);
    } else {
      widget_move_resize(item, r.x, r.y - offset, r.w, r.h);
    }
    vi->index = i;
    vi->view_type = view_type;
    vi->widget = item;
    darray_push(rv->visible_items, vi);
  }

  /* 已挂载项随 offset 重新定位（滑动时需要） */
  {
    uint32_t k = 0;
    for (k = 0; k < rv->visible_items->size; k++) {
      rect_t r;
      visible_item_t* vi = (visible_item_t*)darray_get(rv->visible_items, k);
      if (vi == NULL) continue;
      lm->get_item_rect(lm, widget, vi->index, &r);
      if (lm->is_horizontal) {
        widget_move_resize(vi->widget, r.x - offset, r.y, r.w, r.h);
      } else {
        widget_move_resize(vi->widget, r.x, r.y - offset, r.w, r.h);
      }
    }
  }

  widget_invalidate_force(widget, NULL);
  return RET_OK;
}

/* ---- 惯性 fling ---- */

static ret_t recycle_view_on_fling_timer(const timer_info_t* timer) {
  widget_t* widget = WIDGET(timer->ctx);
  recycle_view_t* rv = RECYCLE_VIEW(widget);
  int32_t before = 0, after = 0;
  if (rv == NULL || rv->layout_manager == NULL) {
    return RET_REMOVE;
  }

  before = recycle_view_main_offset(rv);
  recycle_view_set_main_offset(rv, before + (int32_t)rv->fling_v);
  recycle_view_relayout(widget); /* relayout 内部会 clamp offset */
  after = recycle_view_main_offset(rv);

  rv->fling_v = recycle_fling_next_v(rv->fling_v, RECYCLE_VIEW_FLING_FRICTION);

  /* offset 未变化（已触边）或速度过小 → 停止 */
  if (after == before || fabsf(rv->fling_v) < RECYCLE_VIEW_FLING_MIN_V) {
    rv->fling_timer_id = 0;
    rv->fling_v = 0.0f;
    return RET_REMOVE;
  }
  return RET_REPEAT;
}

static ret_t recycle_view_start_fling(widget_t* widget, float_t v) {
  recycle_view_t* rv = RECYCLE_VIEW(widget);
  return_value_if_fail(rv != NULL, RET_BAD_PARAMS);
  rv->fling_v = v;
  if (fabsf(v) < RECYCLE_VIEW_FLING_MIN_V) {
    rv->fling_v = 0.0f;
    return RET_OK;
  }
  if (rv->fling_timer_id == 0) {
    rv->fling_timer_id =
        widget_add_timer(widget, recycle_view_on_fling_timer, RECYCLE_VIEW_FRAME_INTERVAL_MS);
  }
  return RET_OK;
}

/* ---- 程序滚动（带可选动画） ---- */

static ret_t recycle_view_on_scroll_timer(const timer_info_t* timer) {
  widget_t* widget = WIDGET(timer->ctx);
  recycle_view_t* rv = RECYCLE_VIEW(widget);
  int32_t cur = 0, diff = 0, step = 0;
  if (rv == NULL || rv->layout_manager == NULL) {
    return RET_REMOVE;
  }
  cur = recycle_view_main_offset(rv);
  diff = rv->scroll_target - cur;
  if (diff == 0) {
    rv->scroll_timer_id = 0;
    return RET_REMOVE;
  }
  /* 每帧前进剩余距离的 1/4，最小步长 1，保证收敛 */
  step = diff / 4;
  if (step == 0) {
    step = (diff > 0) ? 1 : -1;
  }
  recycle_view_set_main_offset(rv, cur + step);
  recycle_view_relayout(widget);
  /* 到达目标，或已触边导致 offset 不再变化 → 停止 */
  if (recycle_view_main_offset(rv) == rv->scroll_target ||
      recycle_view_main_offset(rv) == cur) {
    rv->scroll_timer_id = 0;
    return RET_REMOVE;
  }
  return RET_REPEAT;
}

ret_t recycle_view_scroll_to_offset(widget_t* widget, int32_t offset, bool_t animate) {
  recycle_view_t* rv = RECYCLE_VIEW(widget);
  return_value_if_fail(rv != NULL && rv->layout_manager != NULL, RET_BAD_PARAMS);

  /* 停止正在进行的 fling，避免与程序滚动冲突 */
  if (rv->fling_timer_id != 0) {
    timer_remove(rv->fling_timer_id);
    rv->fling_timer_id = 0;
    rv->fling_v = 0.0f;
  }

  if (!animate) {
    recycle_view_set_main_offset(rv, offset);
    return recycle_view_relayout(widget);
  }

  rv->scroll_target = offset; /* relayout 会把实际 offset clamp 到合法范围 */
  if (rv->scroll_timer_id == 0) {
    rv->scroll_timer_id =
        widget_add_timer(widget, recycle_view_on_scroll_timer, RECYCLE_VIEW_FRAME_INTERVAL_MS);
  }
  return RET_OK;
}

ret_t recycle_view_scroll_to_index(widget_t* widget, int32_t index, bool_t animate) {
  recycle_view_t* rv = RECYCLE_VIEW(widget);
  rect_t r;
  int32_t target = 0;
  return_value_if_fail(rv != NULL && rv->layout_manager != NULL, RET_BAD_PARAMS);
  if (index < 0) index = 0;
  rv->layout_manager->get_item_rect(rv->layout_manager, widget, index, &r);
  target = rv->layout_manager->is_horizontal ? r.x : r.y; /* 把该 item 对齐到视口起点 */
  return recycle_view_scroll_to_offset(widget, target, animate);
}

ret_t recycle_view_notify_data_changed(widget_t* widget) {
  recycle_view_t* rv = RECYCLE_VIEW(widget);
  uint32_t k = 0;
  return_value_if_fail(rv != NULL, RET_BAD_PARAMS);
  if (rv->adapter == NULL) {
    return RET_OK;
  }
  /* 先重算总数 */
  rv->item_count = rv->adapter->get_item_count != NULL ? rv->adapter->get_item_count(rv->adapter) : 0;
  /* 对仍可见且仍在数据范围内的项重新绑定（数据变了但壳还在） */
  for (k = 0; k < rv->visible_items->size; k++) {
    visible_item_t* vi = (visible_item_t*)darray_get(rv->visible_items, k);
    if (vi != NULL && vi->index < rv->item_count) {
      rv->adapter->bind_item_view(rv->adapter, vi->widget, vi->index);
    }
  }
  /* 再走标准回收/填充（越界项回收、新进入项填充、offset 夹紧） */
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
  if (rv->scroll_timer_id != 0) {
    timer_remove(rv->scroll_timer_id);
    rv->scroll_timer_id = 0;
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
  recycle_view_t* rv = RECYCLE_VIEW(widget);
  return_value_if_fail(rv != NULL && e != NULL, RET_BAD_PARAMS);
  if (rv->layout_manager == NULL) {
    return RET_OK;
  }

  switch (e->type) {
    case EVT_POINTER_DOWN: {
      pointer_event_t* evt = (pointer_event_t*)e;
      /* 停止正在进行的 fling */
      if (rv->fling_timer_id != 0) {
        timer_remove(rv->fling_timer_id);
        rv->fling_timer_id = 0;
        rv->fling_v = 0.0f;
      }
      rv->dragging = FALSE;
      rv->down_x = evt->x;
      rv->down_y = evt->y;
      rv->down_offset = recycle_view_main_offset(rv);
      velocity_reset(&(rv->velocity));
      velocity_update(&(rv->velocity), e->time, evt->x, evt->y);
      widget_grab(widget->parent, widget);
      break;
    }
    case EVT_POINTER_MOVE: {
      pointer_event_t* evt = (pointer_event_t*)e;
      int32_t delta = rv->layout_manager->is_horizontal ? (evt->x - rv->down_x) : (evt->y - rv->down_y);
      if (!rv->dragging && tk_abs(delta) < RECYCLE_VIEW_DRAG_THRESHOLD) {
        break;
      }
      rv->dragging = TRUE;
      velocity_update(&(rv->velocity), e->time, evt->x, evt->y);
      recycle_view_set_main_offset(rv, rv->down_offset - delta);
      recycle_view_relayout(widget);
      break;
    }
    case EVT_POINTER_UP: {
      pointer_event_t* evt = (pointer_event_t*)e;
      float_t v = 0.0f;
      widget_ungrab(widget->parent, widget);
      if (rv->dragging) {
        velocity_update(&(rv->velocity), e->time, evt->x, evt->y);
        /* velocity 单位是 px/秒；fling 逐帧按 friction 衰减，
         * 取 v*(1-friction) 使总滑动距离≈速度像素值（与 AWTK scroll_view 手感一致）。
         * 负号：拖动方向与 offset 方向相反 */
        v = -(rv->layout_manager->is_horizontal ? rv->velocity.xv : rv->velocity.yv) *
            (1.0f - RECYCLE_VIEW_FLING_FRICTION);
        rv->dragging = FALSE;
        recycle_view_start_fling(widget, v);
      }
      break;
    }
    case EVT_RESIZE: {
      /* 视口尺寸变化：item 尺寸/可见数随之改变，需全量重排 */
      recycle_view_relayout(widget);
      break;
    }
    case EVT_POINTER_DOWN_ABORT: {
      /* 系统级取消（本版 AWTK 无 EVT_POINTER_CANCEL）：松开 grab 并结束拖动，不触发 fling */
      rv->dragging = FALSE;
      widget_ungrab(widget->parent, widget);
      break;
    }
    default:
      break;
  }
  return RET_OK;
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
  rv->visible_items = darray_create(16, visible_item_destroy, NULL);
  rv->recycle_pools = darray_create(4, recycle_pool_destroy, NULL);
  rv->fling_v = 0.0f;
  rv->fling_timer_id = 0;
  rv->dragging = FALSE;
  rv->scroll_animator_id = 0;
  rv->scroll_target = 0;
  rv->scroll_timer_id = 0;
  velocity_reset(&(rv->velocity));

  return widget;
}

recycle_view_t* recycle_view_cast(widget_t* widget) {
  return_value_if_fail(WIDGET_IS_INSTANCE_OF(widget, recycle_view), NULL);
  return (recycle_view_t*)widget;
}
