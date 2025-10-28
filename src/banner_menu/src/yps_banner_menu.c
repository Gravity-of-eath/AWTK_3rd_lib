/**
 * File:   yps_banner_menu.c
 * Author: yps
 * Brief:  yps_banner_menu
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
 * 2025-6-3 yk created
 *
 */

#include "tkc/mem.h"
#include "tkc/utils.h"
#include "yps_banner_menu.h"
// #include "widget.h"
#include "widget_animators/widget_animator_prop.h"
// #include "bllLog.h"
#include "stdio.h"

rect_t *rect_scale_center(rect_t *r, float_t scale)
{
  return_value_if_fail(r != NULL, r);
  /* 计算原始中心点 */
  int32_t x = r->x + (r->w - (r->w * scale)) / 2;
  return rect_create(x, r->y,
                     tk_roundi(r->w * scale), tk_roundi(r->h * scale));
}

// 辅助函数：打印单个节点的详细信息
static void print_node_info(rect_t *node, const char *prefix)
{
  if (node == NULL)
    return;

  printf("print_node_info %s: xywh=(%d, %d, %d, %d)\n",
         prefix,
         node->x,
         node->y,
         node->w,
         node->h);
}

static void init_children_data(yps_banner_menu_t *parent)
{

  if (parent == NULL) //|| node_ptr == NULL
  {
    printf("init_children_data %d\n", __LINE__);
    return;
  }
  int32_t direct_children = widget_count_children(&(parent->widget));
  if (direct_children <= 0)
  {
    printf("init_children_data %d\n", __LINE__);
    return;
  }

  yps_banner_menu_t *yps_banner_menu = (parent);

  yps_banner_menu->childrens = (widget_t **)TKMEM_ALLOC(sizeof(widget_t *) * direct_children);
  for (int32_t i = 0; i < direct_children; i++)
  {
    widget_t *child = widget_get_child(&(parent->widget), i);
    yps_banner_menu->childrens[i] = child;
  }
  rect_t root_rect = rect_init(yps_banner_menu->widget.x, yps_banner_menu->widget.y, yps_banner_menu->widget.w, yps_banner_menu->widget.h);

  ui_tree_node *root_node = ui_tree_node_create(
      yps_banner_menu->widget.name ? yps_banner_menu->widget.name : "Root",
      root_rect,
      0 /* 根节点ID */
  );

  if (!root_node)
  {
    printf("创建根节点失败！\n");
    return;
  }
  yps_banner_menu->child_info = root_node;
  /* 设置用户数据指向原始widget */
  root_node->user_data = yps_banner_menu;

  printf("根节点: %s [%d,%d,%d,%d]\n",
         root_node->name ? root_node->name : "Root",
         root_rect.x, root_rect.y, root_rect.w, root_rect.h);

  /* 递归构建整个树 */
  init_child_recursive(&(parent->widget), root_node, 1);
}

static void scale_widget_group(widget_t *parent, ui_tree_node *child_info, float_t scale)
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
    widget_t *child = widget_get_child(parent, i);
    int32_t new_x = child_info->children[i]->rect.x * scale;
    int32_t new_y = child_info->children[i]->rect.y * scale;
    int32_t new_w = child_info->children[i]->rect.w * scale;
    int32_t new_h = child_info->children[i]->rect.h * scale;
    widget_move_resize(child, new_x, new_y, new_w, new_h);
    if (tk_str_eq(widget_get_type(child), WIDGET_TYPE_LABEL) ||
        tk_str_eq(widget_get_type(child), "shadow_label"))
    {
      widget_set_prop_int(child, "style:normal:font_size",
                          child_info->children[i]->text_size * scale);
      widget_set_prop_int(child, "style:selected:font_size",
                          child_info->children[i]->text_size * scale);
    }
    if (widget_count_children(child) > 0)
    {
      scale_widget_group(child, child_info->children[i], scale);
    }
  }
}

// 修改布局函数调用
static void def_on_layout_vertical(yps_banner_menu_t *parent, widget_t **childrens, int32_t count, int32_t focused, int32_t focus_lossed)
{
  for (int32_t i = 0; i < count; i++)
  {
    if (i == focused)
    {
      childrens[i]->visible = TRUE;
      widget_set_opacity(childrens[i], 255);
      scale_widget_group(childrens[i], parent->child_info->children[i], 1.0f);
      int32_t y = (parent->child_info->rect.h - parent->child_info->children[i]->rect.h) / 2.0f;
      widget_move_resize(childrens[i], parent->child_info->children[i]->rect.x, y, parent->child_info->children[i]->rect.w, parent->child_info->children[i]->rect.h);
    }
    else
    {
      rect_t *r = rect_scale_center(&(parent->child_info->children[i]->rect), parent->scale_ratio);
      scale_widget_group(childrens[i], parent->child_info->children[i], parent->scale_ratio);
      childrens[i]->visible = FALSE;
    }
  }
}

static void def_on_scroll_vertical(yps_banner_menu_t *parent, widget_t **childrens, int32_t count, int32_t lossing, int32_t next, float_t progress)
{
  widget_t *focus_next = childrens[next];
  widget_t *focus_lossing = childrens[lossing];

  printf("def_on_scroll_vertical line:%d\n", __LINE__);

  // 获取两个item的原始高度
  int32_t losing_h = parent->child_info->children[lossing]->rect.h;
  int32_t next_h = parent->child_info->children[next]->rect.h;
  int32_t container_h = parent->child_info->rect.h;

  // 计算中心位置
  float_t center_y = container_h / 2.0f;

  // 设置有效的缩放比例
  float_t scale_ratio = parent->scale_ratio;
  if (scale_ratio <= 0.1f || scale_ratio > 1.0f)
  {
    scale_ratio = 0.8f;
  }

  // 计算缩放变化
  float_t scale_change = 1 - scale_ratio;
  float_t scale_l = 1.0f - (progress * scale_change);
  float_t scale_n = scale_ratio + (progress * scale_change);

  // 确保缩放值有效
  if (scale_l < scale_ratio)
    scale_l = scale_ratio;
  if (scale_n > 1.0f)
    scale_n = 1.0f;

  printf("Scales - losing: %.3f, next: %.3f\n", scale_l, scale_n);

  // 处理lossing item（正在消失的）
  rect_t *r_l = rect_scale_center(&(parent->child_info->children[lossing]->rect), scale_l);
  if (r_l == NULL)
  {
    printf("ERROR: rect_scale_center returned NULL for losing item\n");
    return;
  }

  // 处理next item（正在进入的）
  rect_t *r_n = rect_scale_center(&(parent->child_info->children[next]->rect), scale_n);
  if (r_n == NULL)
  {
    printf("ERROR: rect_scale_center returned NULL for next item\n");
    free(r_l);
    return;
  }

  // 简化位置计算逻辑
  if (parent->next_or_prev)
  {
    // 向上滚动：lossing向下移出，next从上方进入

    // lossing从中心位置向下移动
    r_l->y = center_y - r_l->h / 2.0f + progress * (losing_h + next_h * parent->scale_ratio) / 2.0f;

    // next从上方进入，向下移动到中心位置
    r_n->y = center_y - losing_h / 2.0f - r_n->h + progress * (losing_h + next_h) / 2.0f;
  }
  else
  {
    // 向下滚动：lossing向上移出，next从下方进入

    // lossing从中心位置向上移动
    r_l->y = center_y - r_l->h / 2.0f - progress * (losing_h + next_h * parent->scale_ratio) / 2.0f;

    // next从下方进入，向上移动到中心位置
    r_n->y = center_y + losing_h / 2.0f - progress * (losing_h + next_h) / 2.0f;
  }

  // 应用变换到lossing item
  scale_widget_group(focus_lossing, parent->child_info->children[lossing], scale_l);
  widget_move_resize(focus_lossing, r_l->x, (int32_t)r_l->y, r_l->w, r_l->h);
  widget_set_opacity(focus_lossing, (uint8_t)(scale_l * 255));

  // 应用变换到next item
  scale_widget_group(focus_next, parent->child_info->children[next], scale_n);
  widget_move_resize(focus_next, r_n->x, (int32_t)r_n->y, r_n->w, r_n->h);
  widget_set_opacity(focus_next, (uint8_t)(progress * 255));

  focus_next->visible = TRUE;
  focus_lossing->visible = progress < 1;

  // 调试信息
  float_t losing_bottom = r_l->y + r_l->h;
  float_t next_top = r_n->y;
  float_t gap = fabs(next_top - losing_bottom);

  printf("progress:%.2f, direction:%d\n", progress, parent->next_or_prev);
  printf("losing: %d->%d at y=%d, next: %d->%d at y=%d\n",
         losing_h, r_l->h, r_l->y, next_h, r_n->h, r_n->y);
  printf("gap: %.1f (losing_bottom=%.1f, next_top=%.1f)\n",
         gap, losing_bottom, next_top);
  printf("center_y: %.0f\n", center_y);

  // 清理内存
  free(r_l);
  free(r_n);
}

static layout_manager def_manager_vertical = {
    .on_layout = def_on_layout_vertical,
    .on_scroll = def_on_scroll_vertical};

static ret_t on_anim_function(const timer_info_t *timer)
{

  printf("yps_banner_menu_focus_next 32  \n");
  yps_banner_menu_t *yps_banner_menu = timer->ctx;
  if (yps_banner_menu == NULL)
  {
    printf("ERROR on_anim_function yps_banner_menu==NULL!!!");
    return RET_REMOVE;
  }

  yps_banner_menu->current_progress += yps_banner_menu->step_progress;
  float_t p = 1.0f * yps_banner_menu->current_progress / 100.0f;
  printf("on_anim_function  p=%f current_progress=%d\n", p, yps_banner_menu->current_progress);
  if (p != yps_banner_menu->progress)
  {
    yps_banner_menu->progress = p;
    if (yps_banner_menu->progress < 0.95f)
    {
      if (yps_banner_menu->layout_manager->on_scroll)
      {
        yps_banner_menu->layout_manager->on_scroll(yps_banner_menu, yps_banner_menu->childrens, yps_banner_menu->children_count,
                                                   yps_banner_menu->focus_index, yps_banner_menu->target_index, yps_banner_menu->progress);
        if (yps_banner_menu->listener != NULL && yps_banner_menu->listener->on_scroll != NULL)
        {
          yps_banner_menu->listener->on_scroll(yps_banner_menu, yps_banner_menu->childrens, yps_banner_menu->children_count,
                                               yps_banner_menu->focus_index, yps_banner_menu->target_index, yps_banner_menu->progress);
        }
      }
    }
    else
    {
      yps_banner_menu->on_animation = FALSE;
      if (yps_banner_menu->layout_manager && yps_banner_menu->layout_manager->on_scroll)
      {
        yps_banner_menu->progress = 1.0f;
        yps_banner_menu->layout_manager->on_scroll(yps_banner_menu, yps_banner_menu->childrens, yps_banner_menu->children_count,
                                                   yps_banner_menu->focus_index, yps_banner_menu->target_index, yps_banner_menu->progress);
        if (yps_banner_menu->listener != NULL && yps_banner_menu->listener->on_scroll != NULL)
        {
          yps_banner_menu->listener->on_scroll(yps_banner_menu, yps_banner_menu->childrens, yps_banner_menu->children_count,
                                               yps_banner_menu->focus_index, yps_banner_menu->target_index, yps_banner_menu->progress);
        }
      }
      if (yps_banner_menu->layout_manager && yps_banner_menu->layout_manager->on_layout)
      {
        int32_t temp_index = yps_banner_menu->focus_index;
        yps_banner_menu->focus_index = yps_banner_menu->target_index;
        yps_banner_menu->layout_manager->on_layout(yps_banner_menu, yps_banner_menu->childrens, yps_banner_menu->children_count,
                                                   yps_banner_menu->focus_index, temp_index);
        if (yps_banner_menu->listener != NULL && yps_banner_menu->listener->on_layout != NULL)
        {
          yps_banner_menu->listener->on_layout(yps_banner_menu, yps_banner_menu->childrens, yps_banner_menu->children_count,
                                               yps_banner_menu->focus_index, temp_index);
        }
      }
      return RET_REMOVE;
    }
  }
  widget_invalidate((widget_t *)yps_banner_menu, NULL);
  return RET_REPEAT;
}

static void focus_change(yps_banner_menu_t *yps_banner_menu, int32_t index, bool_t anim)
{
  printf("yps_banner_menu_focus_next 2 anim= %d\n", anim);
  if (anim)
  {
    if (yps_banner_menu->on_animation)
    {
      printf("AAA banner_menu  focus_change on_animation seek this operate!\n");
      return;
    }
    yps_banner_menu->on_animation = TRUE;
    yps_banner_menu->current_progress = 0;
    float_t frame_time = 1000.0f / FPS;
    int32_t frames = yps_banner_menu->animtor_duration * 1.0 / frame_time;
    yps_banner_menu->step_progress = 100.0f / frames;
    if (yps_banner_menu->step_progress <= 1)
    {
      yps_banner_menu->step_progress = 1;
    }
    yps_banner_menu->target_index = index;
    timer_add(on_anim_function, yps_banner_menu, frame_time);
    printf("yps_banner_menu_focus_next 3 anim= %d  step_progress=%d\n", anim, yps_banner_menu->step_progress);
  }
  else
  {
    int32_t temp_index = yps_banner_menu->focus_index;
    yps_banner_menu->focus_index = index;
    yps_banner_menu->target_index = index;
    if (yps_banner_menu->layout_manager && yps_banner_menu->layout_manager->on_layout)
    {
      yps_banner_menu->layout_manager->on_layout(yps_banner_menu, yps_banner_menu->childrens, yps_banner_menu->children_count,
                                                 yps_banner_menu->focus_index, temp_index);
    }
    else
    {
      printf("ERROR !!! no layout_manager.on_layout function");
    }
  }
}

static void refresh(widget_t *widget)
{
  yps_banner_menu_t *yps_banner_menu = YPS_BANNER_MENU(widget);
  return_if_fail(widget != NULL && yps_banner_menu != NULL);
  if (yps_banner_menu->layout_manager)
  {
    if (yps_banner_menu->layout_manager->on_layout)
    {
      yps_banner_menu->layout_manager->on_layout(yps_banner_menu, yps_banner_menu->childrens, yps_banner_menu->children_count,
                                                 yps_banner_menu->focus_index, yps_banner_menu->focus_index);
    }
  }
}

ret_t yps_banner_menu_set_animtor_duration(widget_t *widget, int32_t animtor_duration)
{
  yps_banner_menu_t *yps_banner_menu = YPS_BANNER_MENU(widget);
  return_value_if_fail(yps_banner_menu != NULL, RET_BAD_PARAMS);

  yps_banner_menu->animtor_duration = animtor_duration;

  return RET_OK;
}

ret_t yps_banner_menu_set_focus_index(widget_t *widget, float_t focus_index)
{
  yps_banner_menu_t *yps_banner_menu = YPS_BANNER_MENU(widget);
  return_value_if_fail(yps_banner_menu != NULL, RET_BAD_PARAMS);
  if (focus_index >= 0 && focus_index < yps_banner_menu->children_count)
  {
    focus_change(yps_banner_menu, focus_index, FALSE);
  }
  return RET_OK;
}

int32_t yps_banner_menu_get_focus_index(widget_t *widget)
{
  yps_banner_menu_t *yps_banner_menu = YPS_BANNER_MENU(widget);
  return_value_if_fail(yps_banner_menu != NULL, RET_BAD_PARAMS);
  return yps_banner_menu->focus_index;
}

ret_t yps_banner_menu_set_layout_manager(widget_t *widget, layout_manager *layout_manager)
{
  yps_banner_menu_t *yps_banner_menu = YPS_BANNER_MENU(widget);
  return_value_if_fail(yps_banner_menu != NULL, RET_BAD_PARAMS);
  yps_banner_menu->layout_manager = layout_manager;
  int32_t temp_index = yps_banner_menu->focus_index;
  if (yps_banner_menu->layout_manager)
  {
    if (yps_banner_menu->layout_manager->on_layout)
    {
      yps_banner_menu->layout_manager->on_layout(yps_banner_menu, yps_banner_menu->childrens, yps_banner_menu->children_count,
                                                 yps_banner_menu->focus_index, temp_index);
    }
  }
  return RET_OK;
}

ret_t yps_banner_menu_set_on_scroll_listener(widget_t *widget, layout_manager *listener)
{
  yps_banner_menu_t *yps_banner_menu = YPS_BANNER_MENU(widget);
  return_value_if_fail(yps_banner_menu != NULL, RET_BAD_PARAMS);
  yps_banner_menu->listener = listener;
}

ret_t yps_banner_menu_focus_next(widget_t *widget)
{
  yps_banner_menu_t *yps_banner_menu = YPS_BANNER_MENU(widget);
  return_value_if_fail(yps_banner_menu != NULL, RET_BAD_PARAMS);
  // with  anim
  int32_t focus = yps_banner_menu->focus_index + 1 >= yps_banner_menu->children_count
                      ? 0
                      : yps_banner_menu->focus_index + 1;
  printf("yps_banner_menu_focus_next 1\n");
  yps_banner_menu->next_or_prev = TRUE;
  focus_change(yps_banner_menu, focus, TRUE);

  return RET_OK;
}

ret_t yps_banner_menu_focus_prev(widget_t *widget)
{
  yps_banner_menu_t *yps_banner_menu = YPS_BANNER_MENU(widget);
  return_value_if_fail(yps_banner_menu != NULL, RET_BAD_PARAMS);
  // with  anim
  int32_t focus = yps_banner_menu->focus_index - 1 < 0 ? yps_banner_menu->children_count - 1
                                                       : yps_banner_menu->focus_index - 1;

  yps_banner_menu->next_or_prev = FALSE;
  focus_change(yps_banner_menu, focus, TRUE);
  return RET_OK;
}

static ret_t yps_banner_menu_get_prop(widget_t *widget, const char *name, value_t *v)
{
  yps_banner_menu_t *yps_banner_menu = YPS_BANNER_MENU(widget);
  return_value_if_fail(yps_banner_menu != NULL && name != NULL && v != NULL, RET_BAD_PARAMS);

  if (tk_str_eq(YPS_BANNER_MENU_PROP_ANIMTOR_DURATION, name))
  {
    value_set_int32(v, yps_banner_menu->animtor_duration);
    return RET_OK;
  }
  else if (tk_str_eq(YPS_BANNER_MENU_PROP_FOCUS_INDEX, name))
  {
    value_set_float(v, yps_banner_menu->focus_index);
    return RET_OK;
  }
  else if (tk_str_eq(YPS_BANNER_MENU_PROP_SCALE_RATIO, name))
  {
    value_set_float(v, yps_banner_menu->scale_ratio);
    return RET_OK;
  }

  return RET_NOT_FOUND;
}

static ret_t yps_banner_menu_set_prop(widget_t *widget, const char *name, const value_t *v)
{
  yps_banner_menu_t *yps_banner_menu = YPS_BANNER_MENU(widget);
  return_value_if_fail(widget != NULL && name != NULL && v != NULL, RET_BAD_PARAMS);

  if (tk_str_eq(YPS_BANNER_MENU_PROP_ANIMTOR_DURATION, name))
  {
    yps_banner_menu_set_animtor_duration(widget, value_int32(v));
    return RET_OK;
  }
  else if (tk_str_eq(YPS_BANNER_MENU_PROP_FOCUS_INDEX, name))
  {
    yps_banner_menu_set_focus_index(widget, value_int32(v));
    return RET_OK;
  }
  else if (tk_str_eq(YPS_BANNER_MENU_PROP_REFRESH, name))
  {
    refresh(widget);
    return RET_OK;
  }
  else if (tk_str_eq(YPS_BANNER_MENU_PROP_SCALE_RATIO, name))
  {
    yps_banner_menu->scale_ratio = value_float32(v);
    return RET_OK;
  }

  return RET_NOT_FOUND;
}

// 在销毁函数中释放内存
static ret_t yps_banner_menu_on_destroy(widget_t *widget)
{
  yps_banner_menu_t *yps_banner_menu = YPS_BANNER_MENU(widget);
  return_value_if_fail(widget != NULL && yps_banner_menu != NULL, RET_BAD_PARAMS);
  TKMEM_FREE(yps_banner_menu->childrens);

  return RET_OK;
}

static ret_t yps_banner_menu_on_paint_self(widget_t *widget, canvas_t *c)
{
  yps_banner_menu_t *yps_banner_menu = YPS_BANNER_MENU(widget);

  (void)yps_banner_menu;

  return RET_OK;
}

static ret_t yps_banner_menu_on_event(widget_t *widget, event_t *e)
{
  yps_banner_menu_t *yps_banner_menu = YPS_BANNER_MENU(widget);
  return_value_if_fail(widget != NULL && yps_banner_menu != NULL, RET_BAD_PARAMS);
  if (e->type == EVT_WIDGET_LOAD)
  {
    yps_banner_menu->children_count = widget_count_children(widget);
    yps_banner_menu->childrens = TKMEM_CALLOC(sizeof(widget_t *), yps_banner_menu->children_count);
    init_children_data(yps_banner_menu);
    if (yps_banner_menu->layout_manager->on_layout)
    {
      yps_banner_menu->layout_manager->on_layout(yps_banner_menu, yps_banner_menu->childrens, yps_banner_menu->children_count,
                                                 yps_banner_menu->focus_index, yps_banner_menu->focus_index);
    }
#ifdef DEBUG
    printf("yps_banner_menu yps_banner_menu_on_event !\n");
#endif
  }

  return RET_OK;
}

const char *s_yps_banner_menu_properties[] = {
    YPS_BANNER_MENU_PROP_ANIMTOR_DURATION,
    YPS_BANNER_MENU_PROP_SCALE_RATIO,
    YPS_BANNER_MENU_PROP_FOCUS_INDEX,
    YPS_BANNER_MENU_PROP_REFRESH,
    NULL};

TK_DECL_VTABLE(yps_banner_menu) = {.size = sizeof(yps_banner_menu_t),
                                   .type = WIDGET_TYPE_YPS_BANNER_MENU,
                                   .clone_properties = s_yps_banner_menu_properties,
                                   .persistent_properties = s_yps_banner_menu_properties,
                                   .parent = TK_PARENT_VTABLE(widget),
                                   .create = yps_banner_menu_create,
                                   .on_paint_self = yps_banner_menu_on_paint_self,
                                   .set_prop = yps_banner_menu_set_prop,
                                   .get_prop = yps_banner_menu_get_prop,
                                   .on_event = yps_banner_menu_on_event,
                                   .on_destroy = yps_banner_menu_on_destroy};

widget_t *yps_banner_menu_create(widget_t *parent, xy_t x, xy_t y, wh_t w, wh_t h)
{
  widget_t *widget = widget_create(parent, TK_REF_VTABLE(yps_banner_menu), x, y, w, h);
  yps_banner_menu_t *yps_banner_menu = YPS_BANNER_MENU(widget);
  return_value_if_fail(yps_banner_menu != NULL, NULL);
  yps_banner_menu->focus_index = 0;
  yps_banner_menu->target_index = 0;
  yps_banner_menu->animtor_duration = 500;
  yps_banner_menu->on_animation = FALSE;
  yps_banner_menu->layout_manager = &def_manager_vertical;
  yps_banner_menu->scale_ratio = 0.3f;
  yps_banner_menu->font_scale_ratio = 1.0f;
  return widget;
}

widget_t *yps_banner_menu_cast(widget_t *widget)
{
  return_value_if_fail(WIDGET_IS_INSTANCE_OF(widget, yps_banner_menu), NULL);

  return widget;
}
