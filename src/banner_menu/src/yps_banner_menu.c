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

void scale_widget_group(widget_t *group, rect_t *scale)
{
  if (group == NULL || scale == NULL)
  {
    return;
  }

  /* 计算缩放比例 */
  float scale_x = (float)scale->w / (float)group->w;
  float scale_y = (float)scale->h / (float)group->h;

  /* 缩放组控件本身 */
  widget_move_resize(group, scale->x, scale->y, scale->w, scale->h);

  /* 递归缩放所有子控件 */
  for (int32_t i = 0; i < widget_count_children(group); i++)
  {
    widget_t *child = widget_get_child(group, i);
    /* 保存原始位置和大小 */
    rect_t child_rect = rect_init(child->x, child->y, child->w, child->h);

    /* 计算新的位置和大小 */
    int new_x = scale->x + (int)((child_rect.x - group->x) * scale_x);
    int new_y = scale->y + (int)((child_rect.y - group->y) * scale_y);
    int new_w = (int)(child_rect.w * scale_x);
    int new_h = (int)(child_rect.h * scale_y);

    /* 如果是容器控件，递归缩放其子控件 */
    if (widget_count_children(child) > 0)
    {
      rect_t new_rect = rect_init(new_x, new_y, new_w, new_h);
      scale_widget_group(child, &new_rect);
    }
    else
    {
      widget_move_resize(child, new_x, new_y, new_w, new_h);
    }
  }
}

static void def_on_layout_vertical(yps_banner_menu_t *parent, rect_t *reference_position, widget_t **childrens, int32_t count, int32_t focused, int32_t focus_lossed)
{
  for (int32_t i = 0; i < count; i++)
  {
    if (i == focused)
    {
      childrens[i]->visible = TRUE;
      widget_set_opacity(childrens[i], 255);
      scale_widget_group(childrens[i], reference_position);
    }
    else
    {
      rect_t *r = rect_scale_center(reference_position, parent->scale_ratio);
      scale_widget_group(childrens[i], r);
      TKMEM_FREE(r);
      childrens[i]->visible = FALSE;
    }
  }
}

static void def_on_scroll_vertical(yps_banner_menu_t *parent, rect_t *reference_position, widget_t **childrens, int32_t count, widget_t *focus_lossing, widget_t *focus_next, float_t progress)
{
  ////focus_lossing
  float_t all_move_hight = parent->next_or_prev ? reference_position->h  : -(reference_position->h*(parent->scale_ratio));
  float_t base_y = reference_position->y;
  float_t scale_change = 1 - parent->scale_ratio;
  float_t scale_l = 1.0f - (progress * scale_change);
  rect_t *r_l = rect_scale_center(reference_position, scale_l);
  r_l->y = base_y + (all_move_hight * progress);
  scale_widget_group(focus_lossing, r_l);
  widget_set_opacity(focus_lossing, scale_l * 255);
  // printf("def_on_scroll_vertical scale_l=%f scale_change=%f parent->next_or_prev =%d reference_position->y=%d r_lr=%d,%d,%d,%d,\n", scale_l, scale_change, parent->next_or_prev, reference_position->y, r_l->x, r_l->y, r_l->w, r_l->h);

  // //focus_next

  float_t scale = parent->scale_ratio + (progress * scale_change);
  rect_t *r_n = rect_scale_center(reference_position, scale);
  r_n->y = parent->next_or_prev ? r_l->y - r_n->h : r_l->y + r_l->h;
  scale_widget_group(focus_next, r_n);
  widget_set_opacity(focus_next, scale_l * 255);
  // printf("focus_next scale=%f  r_n->y=%d\n", scale, r_n->y);
  TKMEM_FREE(r_n);
  TKMEM_FREE(r_l);

  focus_next->visible = TRUE;
  focus_lossing->visible = progress < 1;
}

static layout_manager def_manager_vertical = {
    .on_layout = def_on_layout_vertical,
    .on_scroll = def_on_scroll_vertical};

static ret_t on_anim_function(const timer_info_t *timer)
{

  // printf("yps_banner_menu_focus_next 32  \n");
  yps_banner_menu_t *yps_banner_menu = timer->ctx;
  if (yps_banner_menu == NULL)
  {
    printf("ERROR on_anim_function yps_banner_menu==NULL!!!");
    return RET_REMOVE;
  }

  yps_banner_menu->current_progress += yps_banner_menu->step_progress;
  float_t p = 1.0f * yps_banner_menu->current_progress / 100.0f;
  // printf("on_anim_function  p=%f current_progress=%d\n", p, yps_banner_menu->current_progress);
  if (p != yps_banner_menu->progress)
  {
    yps_banner_menu->progress = p;
    if (yps_banner_menu->progress < 0.95f)
    {
      if (yps_banner_menu->layout_manager->on_scroll)
      {
        yps_banner_menu->layout_manager->on_scroll(yps_banner_menu, yps_banner_menu->reference_position, yps_banner_menu->childrens, yps_banner_menu->children_count,
                                                   yps_banner_menu->childrens[yps_banner_menu->focus_index], yps_banner_menu->childrens[yps_banner_menu->target_index], yps_banner_menu->progress);
      }
    }
    else
    {
      yps_banner_menu->on_animation = FALSE;
      if (yps_banner_menu->layout_manager && yps_banner_menu->layout_manager->on_scroll)
      {
        yps_banner_menu->progress = 1.0f;
        yps_banner_menu->layout_manager->on_scroll(yps_banner_menu, yps_banner_menu->reference_position, yps_banner_menu->childrens, yps_banner_menu->children_count,
                                                   yps_banner_menu->childrens[yps_banner_menu->focus_index], yps_banner_menu->childrens[yps_banner_menu->target_index], yps_banner_menu->progress);
      }
      if (yps_banner_menu->layout_manager && yps_banner_menu->layout_manager->on_layout)
      {
        int32_t temp_index = yps_banner_menu->focus_index;
        yps_banner_menu->focus_index = yps_banner_menu->target_index;
        yps_banner_menu->layout_manager->on_layout(yps_banner_menu, yps_banner_menu->reference_position, yps_banner_menu->childrens, yps_banner_menu->children_count,
                                                   yps_banner_menu->focus_index, temp_index);
      }
      return RET_REMOVE;
    }
  }
  widget_invalidate((widget_t*)yps_banner_menu, NULL);
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
      yps_banner_menu->layout_manager->on_layout(yps_banner_menu, yps_banner_menu->reference_position, yps_banner_menu->childrens, yps_banner_menu->children_count,
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
  return_value_if_fail(widget != NULL && yps_banner_menu != NULL, RET_BAD_PARAMS);
  if (yps_banner_menu->layout_manager)
  {
    if (yps_banner_menu->layout_manager->on_layout)
    {
      yps_banner_menu->layout_manager->on_layout(yps_banner_menu, yps_banner_menu->reference_position, yps_banner_menu->childrens, yps_banner_menu->children_count,
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
      yps_banner_menu->layout_manager->on_layout(yps_banner_menu, yps_banner_menu->reference_position, yps_banner_menu->childrens, yps_banner_menu->children_count,
                                                 yps_banner_menu->focus_index, temp_index);
    }
  }
  return RET_OK;
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

static ret_t yps_banner_menu_on_destroy(widget_t *widget)
{
  yps_banner_menu_t *yps_banner_menu = YPS_BANNER_MENU(widget);
  return_value_if_fail(widget != NULL && yps_banner_menu != NULL, RET_BAD_PARAMS);
  TKMEM_FREE(yps_banner_menu->reference_position);
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
    for (int32_t index = 0; index < yps_banner_menu->children_count; index++)
    {
      widget_t *item = widget_get_child(widget, index);
      if (index == 0)
      {
        yps_banner_menu->reference_position->x = item->x;
        yps_banner_menu->reference_position->y = item->y;
        yps_banner_menu->reference_position->w = item->w;
        yps_banner_menu->reference_position->h = item->h;
      }
      yps_banner_menu->childrens[index] = item;
    }
    if (yps_banner_menu->layout_manager->on_layout)
    {
      yps_banner_menu->layout_manager->on_layout(yps_banner_menu, yps_banner_menu->reference_position, yps_banner_menu->childrens, yps_banner_menu->children_count,
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
  yps_banner_menu->reference_position = TKMEM_ZALLOC(rect_t);
  yps_banner_menu->scale_ratio = 0.3f;
  return widget;
}

widget_t *yps_banner_menu_cast(widget_t *widget)
{
  return_value_if_fail(WIDGET_IS_INSTANCE_OF(widget, yps_banner_menu), NULL);

  return widget;
}
