/**
 * File:   recycle_adapter.h
 * Brief:  recycle_view 的数据适配器（使用者实现回调）
 */
#ifndef TK_RECYCLE_ADAPTER_H
#define TK_RECYCLE_ADAPTER_H

#include "base/widget.h"

BEGIN_C_DECLS

typedef struct _recycle_adapter_t recycle_adapter_t;

struct _recycle_adapter_t {
  /* 必填：数据条数 */
  int32_t (*get_item_count)(recycle_adapter_t* adapter);

  /* 可选：第 index 条的 view_type；为 NULL 时所有项视为类型 0 */
  int32_t (*get_item_type)(recycle_adapter_t* adapter, int32_t index);

  /* 必填：为某 view_type 创建一个"空壳"item 控件（不填数据）。parent 即 recycle_view */
  widget_t* (*create_item_view)(recycle_adapter_t* adapter, widget_t* recycle_view, int32_t view_type);

  /* 必填：把第 index 条数据填进 item 控件（item 可能是从回收池复用而来） */
  ret_t (*bind_item_view)(recycle_adapter_t* adapter, widget_t* item, int32_t index);

  /* 可选：item 被回收进池时回调，做解绑/清理；为 NULL 跳过 */
  ret_t (*on_item_recycled)(recycle_adapter_t* adapter, widget_t* item, int32_t view_type);

  /* 可选：adapter 自身资源销毁 */
  ret_t (*on_destroy)(recycle_adapter_t* adapter);

  void* ctx; /* 使用者自定义数据指针 */
};

END_C_DECLS

#endif /*TK_RECYCLE_ADAPTER_H*/
