/**
 * File:   recycle_scale.h
 * Brief:  子树等比缩放辅助（按基线×ratio 绝对重排，幂等；无 GPU 变换）
 */
#ifndef TK_RECYCLE_SCALE_H
#define TK_RECYCLE_SCALE_H
#include "base/widget.h"
BEGIN_C_DECLS
typedef struct _recycle_scale_t recycle_scale_t;
/* 从一棵"满尺寸"子树捕获基线（按 DFS 先序记录每节点 rect + 字号） */
recycle_scale_t* recycle_scale_capture(widget_t* root);
/* 把同构子树 root 的第 k 个 DFS 节点设为 基线[k] × ratio（含字号），绝对、幂等 */
ret_t recycle_scale_apply(recycle_scale_t* s, widget_t* root, float_t ratio);
ret_t recycle_scale_destroy(recycle_scale_t* s);
END_C_DECLS
#endif /*TK_RECYCLE_SCALE_H*/
