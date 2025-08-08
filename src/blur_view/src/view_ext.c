/**
 * File:   view_ext.c
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

#include "view_ext.h"
#include "stdio.h"

static ret_t view_ext_widget_on_paint_children_t(widget_t *widget, canvas_t *c)
{
  view_ext_t *view_ext = VIEW_EXT(widget);
  return_value_if_fail(view_ext != NULL, RET_BAD_PARAMS);
  WIDGET_FOR_EACH_CHILD_BEGIN(widget, iter, i)
  if (view_ext->abort_widget && view_ext->abort_widget == iter)
  {
    // printf("view_ext_create*************abort_widget break= %s***************** \n", widget_get_prop_str(iter, WIDGET_PROP_NAME, "--"));
    break;
  }

  if (!iter->visible)
  {
    iter->dirty = FALSE;
    continue;
  }

  if (!(iter->vt->allow_draw_outside))
  {
    int32_t tolerance = iter->dirty_rect_tolerance;
    int32_t left = c->ox + iter->x - tolerance;
    int32_t top = c->oy + iter->y - tolerance;
    int32_t bottom = top + iter->h + 2 * tolerance;
    int32_t right = left + iter->w + 2 * tolerance;

    if (!canvas_is_rect_in_clip_rect(c, left, top, right, bottom))
    {
      iter->dirty = FALSE;
      continue;
    }
  }

  widget_paint(iter, c);
  WIDGET_FOR_EACH_CHILD_END();
  return RET_OK;
}

TK_DECL_VTABLE(view_ext) = {.size = sizeof(view_ext_t),
                            .type = WIDGET_TYPE_VIEW_EXT,
                            .parent = TK_PARENT_VTABLE(widget),
                            .on_paint_children = view_ext_widget_on_paint_children_t};

void view_ext_set_abort_widget(widget_t *widget, widget_t *abort_widget)
{
  view_ext_t *view_ext = VIEW_EXT(widget);
  return_value_if_fail(view_ext != NULL, RET_BAD_PARAMS);
  view_ext->abort_widget = abort_widget;
  // printf("view_ext_create*************view_ext_set_abort_widget  2 ***************** \n");
}

widget_t *view_ext_create(widget_t *parent, xy_t x, xy_t y, wh_t w, wh_t h)
{
  widget_t *widget = widget_create(parent, TK_REF_VTABLE(view_ext), x, y, w, h);
  printf("view_ext_create****************************** \n");
  return widget;
}

widget_t *view_ext_cast(widget_t *widget)
{
  return_value_if_fail(WIDGET_IS_INSTANCE_OF(widget, view_ext), NULL);
  return widget;
}
