#include "recycle_scale.h"
#include "tkc/mem.h"
#include "tkc/utils.h"
#include "tkc/darray.h"
#include "tkc/rect.h"
#include "base/style.h"

typedef struct _scale_node_t { rect_t rect; int32_t font; } scale_node_t;
struct _recycle_scale_t { scale_node_t* nodes; uint32_t nr; };

/* DFS 先序收集：自身 → 各子；顺序必须与 apply 完全一致 */
static void scale_collect(widget_t* w, darray_t* out) {
  int32_t i = 0, cnt = 0;
  scale_node_t* n = TKMEM_ZALLOC(scale_node_t);
  if (n != NULL) {
    n->rect = rect_init(w->x, w->y, w->w, w->h);
    n->font = (w->astyle != NULL) ? style_get_int(w->astyle, STYLE_ID_FONT_SIZE, 0) : 0;
    darray_push(out, n);
  }
  cnt = widget_count_children(w);
  for (i = 0; i < cnt; i++) {
    scale_collect(widget_get_child(w, i), out);
  }
}

recycle_scale_t* recycle_scale_capture(widget_t* root) {
  recycle_scale_t* s = NULL;
  darray_t* tmp = NULL;
  uint32_t i = 0;
  return_value_if_fail(root != NULL, NULL);
  tmp = darray_create(16, default_destroy, NULL); /* 元素 TKMEM 分配，自动释放 */
  return_value_if_fail(tmp != NULL, NULL);
  scale_collect(root, tmp);
  s = TKMEM_ZALLOC(recycle_scale_t);
  if (s == NULL) { darray_destroy(tmp); return NULL; }
  s->nr = tmp->size;
  s->nodes = (s->nr > 0) ? TKMEM_ZALLOCN(scale_node_t, s->nr) : NULL;
  for (i = 0; i < s->nr; i++) {
    s->nodes[i] = *(scale_node_t*)darray_get(tmp, i);
  }
  darray_destroy(tmp);
  return s;
}

static void scale_apply_dfs(recycle_scale_t* s, widget_t* w, float_t ratio, uint32_t* k) {
  int32_t i = 0, cnt = 0;
  if (*k < s->nr) {
    scale_node_t* n = &s->nodes[*k];
    widget_move_resize(w, (xy_t)(n->rect.x * ratio), (xy_t)(n->rect.y * ratio),
                       (wh_t)(n->rect.w * ratio), (wh_t)(n->rect.h * ratio));
    if (n->font > 0) {
      widget_set_prop_int(w, "style:normal:font_size", (int32_t)(n->font * ratio));
    }
  }
  (*k)++;
  cnt = widget_count_children(w);
  for (i = 0; i < cnt; i++) {
    scale_apply_dfs(s, widget_get_child(w, i), ratio, k);
  }
}

ret_t recycle_scale_apply(recycle_scale_t* s, widget_t* root, float_t ratio) {
  uint32_t k = 0;
  return_value_if_fail(s != NULL && root != NULL, RET_BAD_PARAMS);
  scale_apply_dfs(s, root, ratio, &k);
  return RET_OK;
}

ret_t recycle_scale_destroy(recycle_scale_t* s) {
  if (s != NULL) {
    if (s->nodes != NULL) TKMEM_FREE(s->nodes);
    TKMEM_FREE(s);
  }
  return RET_OK;
}
