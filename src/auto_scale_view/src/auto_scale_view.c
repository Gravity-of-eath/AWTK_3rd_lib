/**
 * File:   yps_cirle_gauge.c
 * Author: 云片松
 * Brief:  圆形表盘光带
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
 * 2025-11-12 wangdongpo created
 *
 */

#include "tkc/mem.h"
#include "tkc/utils.h"
#include "tkc/types_def.h"
#include "auto_scale_view.h"
#include "stdio.h"

#define min(a, b) ((a) < (b) ? (a) : (b))

static void scale_widget_group(widget_t *parent, widget_node_info *child_info, float_t width_scale_ratio, float_t height_scale_ratio)
{
  if (parent == NULL || child_info == NULL)
  {
    return;
    printf("scale_widget_group line: %d\n", __LINE__);
  }

  int32_t direct_children = widget_count_children(parent);
  // printf("scale_widget_group line: %d  direct_children=%d\n", __LINE__, direct_children);
  for (int32_t i = 0; i < direct_children; i++)
  {
    // printf("scale_widget_group line: %d  i=%d\n", __LINE__, i);
    widget_t *child = widget_get_child(parent, i);
    int32_t new_x = child_info->children[i]->rect.x * width_scale_ratio;
    int32_t new_y = child_info->children[i]->rect.y * height_scale_ratio;
    int32_t new_w = child_info->children[i]->rect.w * width_scale_ratio;
    int32_t new_h = child_info->children[i]->rect.h * height_scale_ratio;
    widget_move_resize(child, new_x, new_y, new_w, new_h);
    if (tk_str_eq(widget_get_type(child), WIDGET_TYPE_LABEL) ||
        tk_str_eq(widget_get_type(child), "shadow_label"))
    {
      widget_set_prop_int(child, "style:normal:font_size",
                          child_info->children[i]->text_size * min(width_scale_ratio, height_scale_ratio));
      widget_set_prop_int(child, "style:selected:font_size",
                          child_info->children[i]->text_size * min(width_scale_ratio, height_scale_ratio));
    }
    widget_invalidate(child, NULL);
    if (widget_count_children(child) > 0)
    {
      scale_widget_group(child, child_info->children[i], width_scale_ratio, height_scale_ratio);
    }
  }
}

/**
 * @brief 初始化子控件数据
 * @param widget auto_scale_view控件
 */
static ret_t init_children_data(widget_t *widget)
{
  // printf("init_children_data called line:%d\n",__LINE__);
  auto_scale_view_t *auto_scale_view = AUTO_SCALE_VIEW(widget);
  return_if_fail(auto_scale_view != NULL);

  printf("init_children_data called line:%d\n", __LINE__);
  /* 释放之前的资源 */
  if (auto_scale_view->child_info != NULL)
  {
    widget_node_info_free(auto_scale_view->child_info);
    auto_scale_view->child_info = NULL;
  }

  /* 释放之前的子控件数组 */
  if (auto_scale_view->childrens != NULL)
  {
    TKMEM_FREE(auto_scale_view->childrens);
    auto_scale_view->childrens = NULL;
    auto_scale_view->children_count = 0;
  }

  printf("init_children_data called line:%d\n", __LINE__);
  /* 获取子控件数量 */
  auto_scale_view->children_count = widget_count_children(widget);

  printf("init_children_data called line:%d\n", __LINE__);
  if (auto_scale_view->children_count > 0)
  {
    printf("init_children_data called line:%d\n", __LINE__);
    /* 分配子控件数组内存 */
    auto_scale_view->childrens = TKMEM_ALLOC(sizeof(widget_t *) * auto_scale_view->children_count);
    return_if_fail(auto_scale_view->childrens != NULL);

    printf("init_children_data called line:%d\n", __LINE__);
    /* 填充子控件数组 */
    for (int32_t i = 0; i < auto_scale_view->children_count; i++)
    {
      // printf("init_children_data called line:%d\n",__LINE__);
      auto_scale_view->childrens[i] = widget_get_child(widget, i);
    }

    /* 构建UI树 */
    auto_scale_view->child_info = widget_node_info_create_from_widget_tree(widget);
  }
  printf("init_children_data called line:%d\n", __LINE__);

  /* 初始化缩放比例 */
  auto_scale_view->initial_width = widget_get_prop_int(widget, WIDGET_PROP_W, 0);
  auto_scale_view->initial_height = widget_get_prop_int(widget, WIDGET_PROP_H, 0);

  return RET_OK;
}

void auto_scale_view_set_scale_ratio(widget_t *widget, float_t scale_ratio)
{
  auto_scale_view_t *auto_scale_view = AUTO_SCALE_VIEW(widget);
  return_value_if_fail(auto_scale_view != NULL, RET_BAD_PARAMS);

  auto_scale_view->scale_ratio = scale_ratio;

  /* 如果已经有子控件数据，则立即应用缩放 */
  if (auto_scale_view->childrens != NULL && auto_scale_view->child_info != NULL)
  {
    scale_widget_group(widget, auto_scale_view->child_info, auto_scale_view->scale_ratio, auto_scale_view->scale_ratio);
  }
}

static ret_t auto_scale_view_get_prop(widget_t *widget, const char *name, value_t *v)
{
  auto_scale_view_t *auto_scale_view = AUTO_SCALE_VIEW(widget);
  return_value_if_fail(auto_scale_view != NULL && name != NULL && v != NULL, RET_BAD_PARAMS);

  if (tk_str_eq(AUTO_SCALE_VIEW_PROP_SCALE_RATIO, name))
  {
    value_set_float(v, auto_scale_view->scale_ratio);
    return RET_OK;
  }

  return RET_NOT_FOUND;
}

static ret_t auto_scale_view_set_prop(widget_t *widget, const char *name, const value_t *v)
{
  auto_scale_view_t *auto_scale_view = AUTO_SCALE_VIEW(widget);
  return_value_if_fail(widget != NULL && name != NULL && v != NULL, RET_BAD_PARAMS);

  if (tk_str_eq(AUTO_SCALE_VIEW_PROP_SCALE_RATIO, name))
  {
    auto_scale_view_set_scale_ratio(widget, value_float32(v));
    return RET_OK;
  }
  else if (tk_str_eq(WIDGET_PROP_W, name))
  {

    // auto_scale_view_set_width(widget, value_int32(v));
    if (widget->w > 0 && widget->h > 0)
    {
      float_t width_scale_ratio = (float_t)widget->w / (float_t)auto_scale_view->initial_width;
      float_t height_scale_ratio = (float_t)widget->h / (float_t)auto_scale_view->initial_height;

      scale_widget_group(widget, auto_scale_view->child_info, width_scale_ratio, height_scale_ratio);
    }
    printf("auto_scale_view set width\n");
    return RET_OK;
  }
  else if (tk_str_eq(WIDGET_PROP_H, name))
  {
    // auto_scale_view_set_height(widget, value_int32(v));
    if (widget->w > 0 && widget->h > 0)
    {
      float_t width_scale_ratio = (float_t)widget->w / (float_t)auto_scale_view->initial_width;
      float_t height_scale_ratio = (float_t)widget->h / (float_t)auto_scale_view->initial_height;

      scale_widget_group(widget, auto_scale_view->child_info, width_scale_ratio, height_scale_ratio);
    }
    printf(" auto_scale_view set height\n");
  }

  return RET_NOT_FOUND;
}

static ret_t auto_scale_view_on_event(widget_t *widget, event_t *e)
{
  auto_scale_view_t *auto_scale_view = AUTO_SCALE_VIEW(widget);
  return_value_if_fail(widget != NULL && auto_scale_view != NULL, RET_BAD_PARAMS);

  switch (e->type)
  {
  case EVT_WIDGET_LOAD:
  {
    /* 控件加载完成后初始化子控件数据 */
    init_children_data(widget);
    break;
  }
  case EVT_RESIZE:
  case EVT_MOVE_RESIZE:
  {
    /* 控件大小改变时重新计算缩放比例并应用 */
    if (widget->w > 0 && widget->h > 0 &&
        (widget->w != auto_scale_view->current_width ||
         widget->h != auto_scale_view->current_height))
    {
      auto_scale_view->current_width = widget->w;
      auto_scale_view->current_height = widget->h;
      float_t width_scale_ratio = (float_t)widget->w / (float_t)auto_scale_view->initial_width;
      float_t height_scale_ratio = (float_t)widget->h / (float_t)auto_scale_view->initial_height;
      printf("auto_scale_view_on_event: EVT_RESIZE new (%d,%d) width_scale_ratio=%.2f height_scale_ratio=%.2f\n", widget->w, widget->h, width_scale_ratio, height_scale_ratio);
      scale_widget_group(widget, auto_scale_view->child_info, width_scale_ratio, height_scale_ratio);
    }
    break;
  }
  default:
    break;
  }

  return RET_OK;
}

static ret_t auto_scale_view_on_destroy(widget_t *widget)
{
  auto_scale_view_t *auto_scale_view = AUTO_SCALE_VIEW(widget);
  widget_node_info_free(auto_scale_view->child_info);
  return RET_OK;
}

const char *s_auto_scale_view_properties[] = {
    AUTO_SCALE_VIEW_PROP_SCALE_RATIO,
    AUTO_SCALE_VIEW_PROP_FONT_SCALE_RATIO,
    NULL};

TK_DECL_VTABLE(auto_scale_view) = {.size = sizeof(auto_scale_view_t),
                                   .type = WIDGET_TYPE_AUTO_SCALE_VIEW,
                                   .clone_properties = s_auto_scale_view_properties,
                                   .persistent_properties = s_auto_scale_view_properties,
                                   .parent = TK_PARENT_VTABLE(widget),
                                   .create = auto_scale_view_create,
                                   .set_prop = auto_scale_view_set_prop,
                                   .get_prop = auto_scale_view_get_prop,
                                   .on_event = auto_scale_view_on_event,
                                   .on_destroy = auto_scale_view_on_destroy};

static ret_t auto_scale_view_on_prop_change(void *ctx, event_t *e)
{
  prop_change_event_t *pc_e = (prop_change_event_t *)e;
  widget_t *widget = WIDGET(e->target);
  auto_scale_view_t *auto_scale_view = AUTO_SCALE_VIEW(ctx);
  if (tk_str_eq(pc_e->name, WIDGET_PROP_X) ||
      tk_str_eq(pc_e->name, WIDGET_PROP_Y))
  {
    auto_scale_view->current_width = widget->w;
    auto_scale_view->current_height = widget->h;
    float_t width_scale_ratio = (float_t)widget->w / (float_t)auto_scale_view->initial_width;
    float_t height_scale_ratio = (float_t)widget->h / (float_t)auto_scale_view->initial_height;
    // printf("auto_scale_view_on_prop_change: xy new (%d,%d) width_scale_ratio=%.2f height_scale_ratio=%.2f\n", widget->w, widget->h, width_scale_ratio, height_scale_ratio);
    scale_widget_group(widget, auto_scale_view->child_info, width_scale_ratio, height_scale_ratio);
  }

  return RET_OK;
}

widget_t *auto_scale_view_create(widget_t *parent, xy_t x, xy_t y, wh_t w, wh_t h)
{
  printf("auto_scale_view_create: (%d,%d,%d,%d)\n", x, y, w, h);
  widget_t *widget = widget_create(parent, TK_REF_VTABLE(auto_scale_view), x, y, w, h);
  auto_scale_view_t *auto_scale_view = AUTO_SCALE_VIEW(widget);
  return_value_if_fail(auto_scale_view != NULL, NULL);

  /* 初始化默认值 */
  auto_scale_view->children_count = 0;
  auto_scale_view->childrens = NULL;
  auto_scale_view->scale_ratio = 1.0f;
  auto_scale_view->child_info = NULL;

  widget_on(widget, EVT_PROP_CHANGED, auto_scale_view_on_prop_change, widget);
  return widget;
}

widget_t *auto_scale_view_cast(widget_t *widget)
{
  return_value_if_fail(WIDGET_IS_INSTANCE_OF(widget, auto_scale_view), NULL);

  return widget;
}