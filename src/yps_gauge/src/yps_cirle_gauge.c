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
#include "base/image_manager.h"
#include "yps_cirle_gauge.h"
#include "base/canvas_offline.h"

#define ANCHOR_PX_STR_LEN 2
#define DRAW_POINTER_DIRTY_RECT 1
#define PRE_ROTATED_BITMAPS_NUM 180 // 预旋转0-359度的位图

// 预加载所有角度的旋转指针图片
static ret_t load_pre_rotated_bitmaps(yps_cirle_gauge_t *yps_cirle_gauge)
{
    return_value_if_fail(yps_cirle_gauge != NULL, RET_BAD_PARAMS);

    if (!yps_cirle_gauge->pre_rotated_bitmaps_loaded)
    {
        bitmap_t original_bitmap;
        if (widget_load_image(WIDGET(yps_cirle_gauge), yps_cirle_gauge->pointer_image, &original_bitmap) == RET_OK)
        {
            yps_cirle_gauge->num_pre_rotated_bitmaps = PRE_ROTATED_BITMAPS_NUM;
            yps_cirle_gauge->pre_rotated_bitmaps = (bitmap_t *)TKMEM_ALLOC(sizeof(bitmap_t *) * yps_cirle_gauge->num_pre_rotated_bitmaps);
            for (int i = yps_cirle_gauge->num_pre_rotated_bitmaps; i >= 0; i--)
            {
                bitmap_t *src = &original_bitmap;
                canvas_t *canvas = canvas_offline_create(src->w, src->h, src->format);
                vgcanvas_t *vg = canvas_get_vgcanvas(canvas);
                vgcanvas_save(vg);
                printf("rotate_bitmap :line %d Rotating bitmap by %d degrees\n", __LINE__, i);
                // 设置旋转中心并旋转
                float center_x = src->w / 2.0f;
                float center_y = src->h / 2.0f;
                printf("rotate_bitmap :line %d Rotating bitmap by %d degrees\n", __LINE__, i);
                vgcanvas_translate(vg, center_x, center_y);
                vgcanvas_rotate(vg, (i + 180) * M_PI / 180.0f);
                vgcanvas_translate(vg, -center_x, -center_y);
                printf("rotate_bitmap :line %d Rotating bitmap by %d degrees\n", __LINE__, i);
                // 绘制图像
                vgcanvas_draw_image(vg, src, 0, 0, src->w, src->h, 0, 0, src->w, src->h);
                printf("rotate_bitmap :line %d Rotating bitmap by %d degrees\n", __LINE__, i);
                vgcanvas_restore(vg);
                yps_cirle_gauge->pre_rotated_bitmaps[yps_cirle_gauge->num_pre_rotated_bitmaps - i] = canvas_offline_get_bitmap(canvas);
                //  canvas_offline_destroy(canvas);
            }
        }
    }
    yps_cirle_gauge->pre_rotated_bitmaps_loaded = TRUE;

    return RET_OK;
}

static float value_to_angle(widget_t *widget, int32_t value)
{
    yps_cirle_gauge_t *yps_cirle_gauge = YPS_CIRLE_GAUGE(widget);
    return_value_if_fail(yps_cirle_gauge != NULL, RET_BAD_PARAMS);

    if (value <= yps_cirle_gauge->min_value)
    {
        return yps_cirle_gauge->min_angle;
    }
    else if (value >= yps_cirle_gauge->max_value)
    {
        return yps_cirle_gauge->max_angle;
    }
    else
    {
        int32_t range_value = yps_cirle_gauge->max_value - yps_cirle_gauge->min_value;
        if (range_value <= 0)
            return yps_cirle_gauge->min_angle;

        float_t range_angle = yps_cirle_gauge->max_angle - yps_cirle_gauge->min_angle;
        if (range_angle <= 0)
            return yps_cirle_gauge->min_angle;

        float_t angle = yps_cirle_gauge->min_angle + (value - yps_cirle_gauge->min_value) * range_angle / range_value;
        return angle;
    }
}

ret_t yps_cirle_gauge_set_angle(widget_t *widget, float_t angle)
{
    yps_cirle_gauge_t *yps_cirle_gauge = YPS_CIRLE_GAUGE(widget);
    return_value_if_fail(yps_cirle_gauge != NULL, RET_BAD_PARAMS);
    yps_cirle_gauge->last_angle = yps_cirle_gauge->angle;
    yps_cirle_gauge->angle = angle;
    //   printf("11111111111111111111111111111111111111 \n");
    widget_invalidate_force(widget, NULL);
    //   printf("333333333333333333333333333333333333333333333 \n");
    return RET_OK;
}

ret_t yps_cirle_gauge_set_min_angle(widget_t *widget, float_t min_angle)
{
    yps_cirle_gauge_t *yps_cirle_gauge = YPS_CIRLE_GAUGE(widget);
    return_value_if_fail(yps_cirle_gauge != NULL, RET_BAD_PARAMS);

    yps_cirle_gauge->min_angle = min_angle;
    return RET_OK;
}

ret_t yps_cirle_gauge_set_max_angle(widget_t *widget, float_t max_angle)
{
    yps_cirle_gauge_t *yps_cirle_gauge = YPS_CIRLE_GAUGE(widget);
    return_value_if_fail(yps_cirle_gauge != NULL, RET_BAD_PARAMS);

    yps_cirle_gauge->max_angle = max_angle;
    return RET_OK;
}

ret_t yps_cirle_gauge_set_min_value(widget_t *widget, int32_t min_value)
{
    yps_cirle_gauge_t *yps_cirle_gauge = YPS_CIRLE_GAUGE(widget);
    return_value_if_fail(yps_cirle_gauge != NULL, RET_BAD_PARAMS);

    yps_cirle_gauge->min_value = min_value;
    return RET_OK;
}

ret_t yps_cirle_gauge_set_max_value(widget_t *widget, int32_t max_value)
{
    yps_cirle_gauge_t *yps_cirle_gauge = YPS_CIRLE_GAUGE(widget);
    return_value_if_fail(yps_cirle_gauge != NULL, RET_BAD_PARAMS);
    yps_cirle_gauge->max_value = max_value;
    return RET_OK;
}

ret_t yps_cirle_gauge_set_r1(widget_t *widget, int32_t r1)
{
    yps_cirle_gauge_t *yps_cirle_gauge = YPS_CIRLE_GAUGE(widget);
    return_value_if_fail(yps_cirle_gauge != NULL, RET_BAD_PARAMS);

    yps_cirle_gauge->r1 = r1;

    return RET_OK;
}

ret_t yps_cirle_gauge_set_r2(widget_t *widget, int32_t r2)
{
    yps_cirle_gauge_t *yps_cirle_gauge = YPS_CIRLE_GAUGE(widget);
    return_value_if_fail(yps_cirle_gauge != NULL, RET_BAD_PARAMS);

    yps_cirle_gauge->r2 = r2;

    return RET_OK;
}

ret_t yps_cirle_gauge_set_image1(widget_t *widget, const char *image1)
{
    yps_cirle_gauge_t *yps_cirle_gauge = YPS_CIRLE_GAUGE(widget);
    return_value_if_fail(yps_cirle_gauge != NULL, RET_BAD_PARAMS);
    yps_cirle_gauge->image1 = tk_str_copy(yps_cirle_gauge->image1, image1);
    return RET_OK;
}

ret_t yps_cirle_gauge_set_image2(widget_t *widget, const char *image2)
{
    yps_cirle_gauge_t *yps_cirle_gauge = YPS_CIRLE_GAUGE(widget);
    return_value_if_fail(yps_cirle_gauge != NULL, RET_BAD_PARAMS);
    yps_cirle_gauge->image2 = tk_str_copy(yps_cirle_gauge->image2, image2);
    return RET_OK;
}

ret_t yps_cirle_gauge_set_total_degree(widget_t *widget, float_t total_degree)
{
    yps_cirle_gauge_t *yps_cirle_gauge = YPS_CIRLE_GAUGE(widget);
    return_value_if_fail(yps_cirle_gauge != NULL, RET_BAD_PARAMS);

    yps_cirle_gauge->total_degree = total_degree;

    return RET_OK;
}

ret_t yps_cirle_gauge_set_critical(widget_t *widget, float_t critical)
{
    yps_cirle_gauge_t *yps_cirle_gauge = YPS_CIRLE_GAUGE(widget);
    return_value_if_fail(yps_cirle_gauge != NULL, RET_BAD_PARAMS);

    yps_cirle_gauge->critical = critical;

    return RET_OK;
}

ret_t yps_cirle_gauge_set_anchor_x(widget_t *widget, int32_t anchor_x)
{
    yps_cirle_gauge_t *yps_cirle_gauge = YPS_CIRLE_GAUGE(widget);
    return_value_if_fail(yps_cirle_gauge != NULL, RET_BAD_PARAMS);

    yps_cirle_gauge->anchor_x = anchor_x;
    return RET_OK;
}

ret_t yps_cirle_gauge_set_anchor_y(widget_t *widget, int32_t anchor_y)
{
    yps_cirle_gauge_t *yps_cirle_gauge = YPS_CIRLE_GAUGE(widget);
    return_value_if_fail(yps_cirle_gauge != NULL, RET_BAD_PARAMS);
    yps_cirle_gauge->anchor_y = anchor_y;
    return RET_OK;
}

ret_t yps_cirle_gauge_set_pointer_image(widget_t *widget, const char *pointer_image)
{
    yps_cirle_gauge_t *yps_cirle_gauge = YPS_CIRLE_GAUGE(widget);
    return_value_if_fail(yps_cirle_gauge != NULL, RET_BAD_PARAMS);
    yps_cirle_gauge->pointer_image = tk_str_copy(yps_cirle_gauge->pointer_image, pointer_image);

    // 如果预旋转位图已经加载，重新加载它们
    if (yps_cirle_gauge->pre_rotated_bitmaps_loaded)
    {
        // load_pre_rotated_bitmaps(yps_cirle_gauge);
    }

    return RET_OK;
}

ret_t yps_cirle_gauge_set_pointer_image2(widget_t *widget, const char *pointer_image2)
{
    yps_cirle_gauge_t *yps_cirle_gauge = YPS_CIRLE_GAUGE(widget);
    return_value_if_fail(yps_cirle_gauge != NULL, RET_BAD_PARAMS);
    yps_cirle_gauge->pointer_image2 = tk_str_copy(yps_cirle_gauge->pointer_image2, pointer_image2);

    // 如果预旋转位图已经加载，重新加载它们
    if (yps_cirle_gauge->pre_rotated_bitmaps_loaded)
    {
        // load_pre_rotated_bitmaps(yps_cirle_gauge);
    }

    return RET_OK;
}

ret_t yps_cirle_gauge_set_pointer_offset_x(widget_t *widget, int32_t pointer_offset_x)
{
    yps_cirle_gauge_t *yps_cirle_gauge = YPS_CIRLE_GAUGE(widget);
    return_value_if_fail(yps_cirle_gauge != NULL, RET_BAD_PARAMS);
    yps_cirle_gauge->pointer_offset_x = pointer_offset_x;
    return RET_OK;
}

ret_t yps_cirle_gauge_set_pointer_offset_y(widget_t *widget, int32_t pointer_offset_y)
{
    yps_cirle_gauge_t *yps_cirle_gauge = YPS_CIRLE_GAUGE(widget);
    return_value_if_fail(yps_cirle_gauge != NULL, RET_BAD_PARAMS);
    yps_cirle_gauge->pointer_offset_y = pointer_offset_y;
    return RET_OK;
}

static ret_t yps_cirle_gauge_get_prop(widget_t *widget, const char *name, value_t *v)
{
    yps_cirle_gauge_t *yps_cirle_gauge = YPS_CIRLE_GAUGE(widget);
    return_value_if_fail(yps_cirle_gauge != NULL && name != NULL && v != NULL, RET_BAD_PARAMS);

    if (tk_str_eq("value", name))
    {
        value_set_int32(v, yps_cirle_gauge->value);
        return RET_OK;
    }
    else if (tk_str_eq(YPS_CIRLE_GAUGE_PROP_ANGLE, name))
    {
        value_set_float(v, yps_cirle_gauge->angle);
        return RET_OK;
    }
    else if (tk_str_eq(YPS_CIRLE_GAUGE_PROP_MIN_ANGLE, name))
    {
        value_set_float(v, yps_cirle_gauge->min_angle);
        return RET_OK;
    }
    else if (tk_str_eq(YPS_CIRLE_GAUGE_PROP_MAX_ANGLE, name))
    {
        value_set_float(v, yps_cirle_gauge->max_angle);
        return RET_OK;
    }
    else if (tk_str_eq(YPS_CIRLE_GAUGE_PROP_MIN_VALUE, name))
    {
        value_set_int32(v, yps_cirle_gauge->min_value);
        return RET_OK;
    }
    else if (tk_str_eq(YPS_CIRLE_GAUGE_PROP_MAX_VALUE, name))
    {
        value_set_int32(v, yps_cirle_gauge->max_value);
        return RET_OK;
    }
    else if (tk_str_eq(YPS_CIRLE_GAUGE_PROP_R1, name))
    {
        value_set_int32(v, yps_cirle_gauge->r1);
        return RET_OK;
    }
    else if (tk_str_eq(YPS_CIRLE_GAUGE_PROP_R2, name))
    {
        value_set_int32(v, yps_cirle_gauge->r2);
        return RET_OK;
    }
    else if (tk_str_eq(YPS_CIRLE_GAUGE_PROP_IMAGE1, name))
    {
        value_set_str(v, yps_cirle_gauge->image1);
        return RET_OK;
    }
    else if (tk_str_eq(YPS_CIRLE_GAUGE_PROP_IMAGE2, name))
    {
        value_set_str(v, yps_cirle_gauge->image2);
        return RET_OK;
    }
    else if (tk_str_eq(YPS_CIRLE_GAUGE_PROP_TOTAL_DEGREE, name))
    {
        value_set_float(v, yps_cirle_gauge->total_degree);
        return RET_OK;
    }
    else if (tk_str_eq(YPS_CIRLE_GAUGE_PROP_CRITICAL, name))
    {
        value_set_float(v, yps_cirle_gauge->critical);
        return RET_OK;
    }
    else if (tk_str_eq(YPS_CIRLE_GAUGE_PROP_ANCHOR_X, name))
    {
        value_set_int(v, yps_cirle_gauge->anchor_x);
        return RET_OK;
    }
    else if (tk_str_eq(YPS_CIRLE_GAUGE_PROP_ANCHOR_Y, name))
    {
        value_set_int(v, yps_cirle_gauge->anchor_y);
        return RET_OK;
    }
    else if (tk_str_eq(YPS_CIRLE_GAUGE_PROP_POINTER_IMAGE, name))
    {
        value_set_str(v, yps_cirle_gauge->pointer_image);
        return RET_OK;
    }
    else if (tk_str_eq(YPS_CIRLE_GAUGE_PROP_POINTER_IMAGE2, name))
    {
        value_set_str(v, yps_cirle_gauge->pointer_image2);
        return RET_OK;
    }
    else if (tk_str_eq(YPS_CIRLE_GAUGE_PROP_POINTER_OFFSET_X, name))
    {
        value_set_int(v, yps_cirle_gauge->pointer_offset_x);
    }
    else if (tk_str_eq(YPS_CIRLE_GAUGE_PROP_POINTER_OFFSET_Y, name))
    {
        value_set_int(v, yps_cirle_gauge->pointer_offset_y);
        return RET_OK;
    }

    return RET_NOT_FOUND;
}

static ret_t yps_cirle_gauge_set_prop(widget_t *widget, const char *name, const value_t *v)
{
    yps_cirle_gauge_t *yps_cirle_gauge = YPS_CIRLE_GAUGE(widget);
    return_value_if_fail(widget != NULL && name != NULL && v != NULL, RET_BAD_PARAMS);

    if (tk_str_eq("value", name))
    {
        yps_cirle_gauge->value = value_int32(v);
        if (yps_cirle_gauge->value > yps_cirle_gauge->max_value)
        {
            yps_cirle_gauge->value = yps_cirle_gauge->max_value;
        }
        else if (yps_cirle_gauge->value < yps_cirle_gauge->min_value)
        {
            yps_cirle_gauge->value = yps_cirle_gauge->min_value;
        }
        yps_cirle_gauge_set_angle(widget, value_to_angle(widget, yps_cirle_gauge->value));
        return RET_OK;
    }
    else if (tk_str_eq(YPS_CIRLE_GAUGE_PROP_ANGLE, name))
    {
        yps_cirle_gauge_set_angle(widget, value_float(v));
        return RET_OK;
    }
    else if (tk_str_eq(YPS_CIRLE_GAUGE_PROP_MIN_ANGLE, name))
    {
        yps_cirle_gauge_set_min_angle(widget, value_float(v));
        return RET_OK;
    }
    else if (tk_str_eq(YPS_CIRLE_GAUGE_PROP_MAX_ANGLE, name))
    {
        yps_cirle_gauge_set_max_angle(widget, value_float(v));
        return RET_OK;
    }
    else if (tk_str_eq(YPS_CIRLE_GAUGE_PROP_MIN_VALUE, name))
    {
        yps_cirle_gauge_set_min_value(widget, value_int32(v));
        return RET_OK;
    }
    else if (tk_str_eq(YPS_CIRLE_GAUGE_PROP_MAX_VALUE, name))
    {
        yps_cirle_gauge_set_max_value(widget, value_int32(v));
        return RET_OK;
    }
    else if (tk_str_eq(YPS_CIRLE_GAUGE_PROP_R1, name))
    {
        yps_cirle_gauge_set_r1(widget, value_int32(v));
        return RET_OK;
    }
    else if (tk_str_eq(YPS_CIRLE_GAUGE_PROP_R2, name))
    {
        yps_cirle_gauge_set_r2(widget, value_int32(v));
        return RET_OK;
    }
    else if (tk_str_eq(YPS_CIRLE_GAUGE_PROP_IMAGE1, name))
    {
        yps_cirle_gauge_set_image1(widget, value_str(v));
        return RET_OK;
    }
    else if (tk_str_eq(YPS_CIRLE_GAUGE_PROP_IMAGE2, name))
    {
        yps_cirle_gauge_set_image2(widget, value_str(v));
        return RET_OK;
    }
    else if (tk_str_eq(YPS_CIRLE_GAUGE_PROP_TOTAL_DEGREE, name))
    {
        yps_cirle_gauge_set_total_degree(widget, value_float(v));
        return RET_OK;
    }
    else if (tk_str_eq(YPS_CIRLE_GAUGE_PROP_CRITICAL, name))
    {
        yps_cirle_gauge_set_critical(widget, value_float(v));
        return RET_OK;
    }
    else if (tk_str_eq(YPS_CIRLE_GAUGE_PROP_ANCHOR_X, name))
    {
        yps_cirle_gauge_set_anchor_x(widget, value_int32(v));
        return RET_OK;
    }
    else if (tk_str_eq(YPS_CIRLE_GAUGE_PROP_ANCHOR_Y, name))
    {
        yps_cirle_gauge_set_anchor_y(widget, value_int32(v));
        return RET_OK;
    }
    else if (tk_str_eq(YPS_CIRLE_GAUGE_PROP_POINTER_IMAGE, name))
    {
        yps_cirle_gauge_set_pointer_image(widget, value_str(v));
        return RET_OK;
    }
    else if (tk_str_eq(YPS_CIRLE_GAUGE_PROP_POINTER_IMAGE2, name))
    {
        yps_cirle_gauge_set_pointer_image2(widget, value_str(v));
        return RET_OK;
    }
    else if (tk_str_eq(YPS_CIRLE_GAUGE_PROP_POINTER_OFFSET_X, name))
    {
        yps_cirle_gauge_set_pointer_offset_x(widget, value_int32(v));
        return RET_OK;
    }
    else if (tk_str_eq(YPS_CIRLE_GAUGE_PROP_POINTER_OFFSET_Y, name))
    {
        yps_cirle_gauge_set_pointer_offset_y(widget, value_int32(v));
        return RET_OK;
    }

    return RET_NOT_FOUND;
}

static ret_t yps_cirle_gauge_on_destroy(widget_t *widget)
{
    yps_cirle_gauge_t *yps_cirle_gauge = YPS_CIRLE_GAUGE(widget);
    return_value_if_fail(widget != NULL && yps_cirle_gauge != NULL, RET_BAD_PARAMS);

    tk_free(yps_cirle_gauge->image1);
    tk_free(yps_cirle_gauge->image2);
    tk_free(yps_cirle_gauge->pointer_image);
    tk_free(yps_cirle_gauge->pointer_image2);
    yps_cirle_gauge->image1 = NULL;
    yps_cirle_gauge->image2 = NULL;
    yps_cirle_gauge->pointer_image = NULL;
    yps_cirle_gauge->pointer_image2 = NULL;

    // 释放预旋转位图数组
    if (yps_cirle_gauge->pre_rotated_bitmaps_loaded && yps_cirle_gauge->pre_rotated_bitmaps != NULL)
    {
        for (int i = 0; i < yps_cirle_gauge->num_pre_rotated_bitmaps; i++)
        {
            bitmap_destroy(&yps_cirle_gauge->pre_rotated_bitmaps[i]);
        }
        TKMEM_FREE(yps_cirle_gauge->pre_rotated_bitmaps);
        yps_cirle_gauge->pre_rotated_bitmaps = NULL;
        yps_cirle_gauge->num_pre_rotated_bitmaps = 0;
        yps_cirle_gauge->pre_rotated_bitmaps_loaded = FALSE;
    }

    return RET_OK;
}

static ret_t yps_cirle_gauge_on_paint_self(widget_t *widget, canvas_t *c)
{
    yps_cirle_gauge_t *yps_cirle_gauge = YPS_CIRLE_GAUGE(widget);

    vgcanvas_t *vg = canvas_get_vgcanvas(c);

    // 约定 6点方向为0° 12点方向为180° 9点方向为90° 3点方向为270°

    /// 角度转弧度 因为控件6点方向为0度 这里需要减去90°
    float start_arc = (yps_cirle_gauge->min_angle - 90) * M_PI / 180.0;
    float end_arc = (yps_cirle_gauge->angle - 90) * M_PI / 180.0;

    int32_t anchor_x = yps_cirle_gauge->anchor_x;
    int32_t anchor_y = yps_cirle_gauge->anchor_y;
    // printf("[%d, %d] anchor_x = %d , anchor_y = %d\n", widget->w, widget->h, anchor_x, anchor_y);

    // printf("angle: %f, rotation: %f \n", yps_cirle_gauge->angle, rotation);
    { // 绘制闭合扇形路径，贴图方式绘制光带
        bitmap_t bmp;
        char *image_name = yps_cirle_gauge->image1;
        if (strlen(yps_cirle_gauge->image2) > 0 && yps_cirle_gauge->value >= yps_cirle_gauge->critical)
        {
            image_name = yps_cirle_gauge->image2;
        }
        if (RET_OK != widget_load_image(widget, image_name, &bmp))
        {
            printf("%s load image (%s) failed\n", widget->name ? widget->name : widget->vt->type, yps_cirle_gauge->image1);
            return RET_OK;
        }
        vgcanvas_save(vg);
        vgcanvas_translate(vg, c->ox, c->oy);
        vgcanvas_begin_path(vg);
        vgcanvas_arc(vg, anchor_x, anchor_y, yps_cirle_gauge->r1, start_arc, end_arc, FALSE);
        vgcanvas_arc(vg, anchor_x, anchor_y, yps_cirle_gauge->r2, end_arc, start_arc, TRUE);
        vgcanvas_close_path(vg);
        vgcanvas_paint(vg, FALSE, &bmp);

        // vgcanvas_stroke(vg);
        vgcanvas_restore(vg);

        // 使用预旋转的BMP数组来绘制指针，避免实时旋转
        if (tk_strlen(yps_cirle_gauge->pointer_image) > 0)
        {
            // 确保预旋转位图已经加载
            if (!yps_cirle_gauge->pre_rotated_bitmaps_loaded)
            {
                load_pre_rotated_bitmaps(yps_cirle_gauge);
            }

            // 获取当前角度对应的预旋转BMP

            // int angle_index = (int)fmod(fmod(yps_cirle_gauge->angle, 360.0f) + 360.0f, 360.0f); // 确保在0-359范围内
            int32_t angle_index = round(yps_cirle_gauge->angle) * -1;
            if (yps_cirle_gauge->pre_rotated_bitmaps_loaded &&
                angle_index >= 0 && angle_index < yps_cirle_gauge->num_pre_rotated_bitmaps)
            {
                bitmap_t *rotated_bitmap = yps_cirle_gauge->pre_rotated_bitmaps[angle_index];
                float rotation = (yps_cirle_gauge->angle + 180.0f) * M_PI / 180.0f;
                int32_t rr = (int32_t)(yps_cirle_gauge->r1 + yps_cirle_gauge->r2) / 2.0f;
                // vgcanvas_save(vg);
                canvas_save(c);
                // vgcanvas_translate(vg, c->ox, c->oy);
                canvas_translate(vg, c->ox, c->oy);
                // vgcanvas_translate(vg, anchor_x, anchor_y); // 平移到控件中心
                int32_t xx = anchor_x - (rr * sin(rotation)) - rotated_bitmap->w / 2;
                int32_t yy =  anchor_y + (rr * cos(rotation)) - rotated_bitmap->h / 2;
                // vgcanvas_draw_image(vg, rotated_bitmap, 0, 0, rotated_bitmap->w, rotated_bitmap->h,
                //                     xx, yy,
                //                     rotated_bitmap->w, rotated_bitmap->h);
                canvas_draw_image_ex2(c, rotated_bitmap, IMAGE_DRAW_ICON, rect_create(00, 00, rotated_bitmap->w, rotated_bitmap->h),rect_create(xx, yy, rotated_bitmap->w, rotated_bitmap->h));
                // printf("Drawing pointer at angle 3333 index: %d anchor_x=%d anchor_y=%d rotation=%f  xx=%d yy=%d  angle=%f\n", angle_index, anchor_x, anchor_y, rotation, xx, yy, (yps_cirle_gauge->angle + 180.0f));
                // vgcanvas_restore(vg);
                canvas_restore(c);
            }
        }
    }

    // { // 绘制脏矩形
    //     vgcanvas_save(vg);
    //     vgcanvas_translate(vg, c->ox, c->oy);
    //     vgcanvas_set_stroke_color(vg, color_init(255, 0, 0, 255));
    //     vgcanvas_rect(vg, yps_cirle_gauge->dirty_rect.x + 2, yps_cirle_gauge->dirty_rect.y + 2, yps_cirle_gauge->dirty_rect.w - 4, yps_cirle_gauge->dirty_rect.h - 4);
    //     vgcanvas_stroke(vg);
    //     vgcanvas_set_stroke_color(vg, color_init(0, 255, 0, 255));
    //     vgcanvas_rect(vg, yps_cirle_gauge->pointer_dirty_rect.x, yps_cirle_gauge->pointer_dirty_rect.y, yps_cirle_gauge->pointer_dirty_rect.w, yps_cirle_gauge->pointer_dirty_rect.h);
    //     vgcanvas_stroke(vg);
    //     vgcanvas_restore(vg);
    // }

    // { /// 绘制圆心
    //     vgcanvas_save(vg);
    //     vgcanvas_translate(vg, c->ox, c->oy);
    //     vgcanvas_set_stroke_color(vg, color_init(255, 0, 0, 255));
    //     vgcanvas_set_line_width(vg, 1);
    //     vgcanvas_ellipse(vg, anchor_x, anchor_y, 3, 3);
    //     vgcanvas_stroke(vg);
    //     vgcanvas_restore(vg);
    // }
    return RET_OK;
}

/**
 * 合并两个矩形为一个更大的矩形
 */
static rect_t merge_rects(rect_t rect1, rect_t rect2)
{
    rect_t merged = {0};

    if (rect1.w == 0 || rect1.h == 0)
    {
        return rect2;
    }

    if (rect2.w == 0 || rect2.h == 0)
    {
        return rect1;
    }

    float min_x = tk_min(rect1.x, rect2.x);
    float min_y = tk_min(rect1.y, rect2.y);
    float max_x = tk_max(rect1.x + rect1.w, rect2.x + rect2.w);
    float max_y = tk_max(rect1.y + rect1.h, rect2.y + rect2.h);

    merged.x = min_x;
    merged.y = min_y;
    merged.w = max_x - min_x;
    merged.h = max_y - min_y;

    return merged;
}

static ret_t yps_cirle_gauge_invalidate(widget_t *widget, const rect_t *r)
{
    yps_cirle_gauge_t *yps_cirle_gauge = YPS_CIRLE_GAUGE(widget);
    return_value_if_fail(widget != NULL && yps_cirle_gauge != NULL, RET_BAD_PARAMS);

    // 计算角度差
    float angle_diff = yps_cirle_gauge->angle - yps_cirle_gauge->last_angle;

    // 如果角度没有变化，不需要重绘
    if (fabs(angle_diff) < 0.1f)
    {
        return RET_OK;
    }

    // 计算表盘光带的脏矩形
    int32_t anchor_x = yps_cirle_gauge->anchor_x;
    int32_t anchor_y = yps_cirle_gauge->anchor_y;
    int32_t r1 = yps_cirle_gauge->r1;
    int32_t r2 = yps_cirle_gauge->r2;

    // 计算需要重绘的扇形区域 - 从上一次角度到当前角度
    float start_angle = yps_cirle_gauge->last_angle - 90;
    float end_angle = yps_cirle_gauge->angle - 90;

    // 确保起始角度小于结束角度
    if (start_angle > end_angle)
    {
        float temp = start_angle;
        start_angle = end_angle;
        end_angle = temp;
    }

    // 转换为弧度 - 根据角度系统：6点方向为0°，12点方向为180°，9点方向为90°，3点方向为270°
    float start_arc = start_angle * M_PI / 180.0;
    float end_arc = end_angle * M_PI / 180.0;

    // 计算扇形区域的边界 - 初始化为第一个点的坐标
    float min_x = anchor_x + r1 * cos(start_arc);
    float min_y = anchor_y + r1 * sin(start_arc);
    float max_x = min_x;
    float max_y = min_y;

    // 计算扇形光带区域的四个角点
    float points[8];
    points[0] = anchor_x + r1 * cos(start_arc); // 外环起始点x
    points[1] = anchor_y + r1 * sin(start_arc); // 外环起始点y
    points[2] = anchor_x + r1 * cos(end_arc);   // 外环结束点x
    points[3] = anchor_y + r1 * sin(end_arc);   // 外环结束点y
    points[4] = anchor_x + r2 * cos(start_arc); // 内环起始点x
    points[5] = anchor_y + r2 * sin(start_arc); // 内环起始点y
    points[6] = anchor_x + r2 * cos(end_arc);   // 内环结束点x
    points[7] = anchor_y + r2 * sin(end_arc);   // 内环结束点y

    // 找出最小和最大的x,y坐标
    for (int i = 0; i < 8; i += 2)
    {
        if (points[i] < min_x)
            min_x = points[i];
        if (points[i] > max_x)
            max_x = points[i];
        if (points[i + 1] < min_y)
            min_y = points[i + 1];
        if (points[i + 1] > max_y)
            max_y = points[i + 1];
    }

    // 检查扇形是否跨越0度（6点方向，底部）
    // 如果跨越，需要额外考虑0度方向的点
    if ((start_angle <= 0 && end_angle >= 0) || (start_angle <= 360 && end_angle >= 360))
    {
        // 添加0度方向的点（底部）
        float bottom_y = anchor_y + r1;
        if (bottom_y > max_y)
            max_y = bottom_y;
        if (bottom_y < min_y)
            min_y = bottom_y;

        float bottom_y_inner = anchor_y + r2;
        if (bottom_y_inner > max_y)
            max_y = bottom_y_inner;
        if (bottom_y_inner < min_y)
            min_y = bottom_y_inner;
    }

    // 检查扇形是否跨越90度（9点方向，左侧）
    if ((start_angle <= 90 && end_angle >= 90))
    {
        // 添加90度方向的点（左侧）
        float left_x = anchor_x - r1;
        if (left_x > max_x)
            max_x = left_x;
        if (left_x < min_x)
            min_x = left_x;

        float left_x_inner = anchor_x - r2;
        if (left_x_inner > max_x)
            max_x = left_x_inner;
        if (left_x_inner < min_x)
            min_x = left_x_inner;
    }

    // 检查扇形是否跨越180度（12点方向，顶部）
    if ((start_angle <= 180 && end_angle >= 180))
    {
        // 添加180度方向的点（顶部）
        float top_y = anchor_y - r1;
        if (top_y > max_y)
            max_y = top_y;
        if (top_y < min_y)
            min_y = top_y;

        float top_y_inner = anchor_y - r2;
        if (top_y_inner > max_y)
            max_y = top_y_inner;
        if (top_y_inner < min_y)
            min_y = top_y_inner;
    }

    // 检查扇形是否跨越270度（3点方向，右侧）
    if ((start_angle <= 270 && end_angle >= 270))
    {
        // 添加270度方向的点（右侧）
        float right_x = anchor_x + r1;
        if (right_x > max_x)
            max_x = right_x;
        if (right_x < min_x)
            min_x = right_x;

        float right_x_inner = anchor_x + r2;
        if (right_x_inner > max_x)
            max_x = right_x_inner;
        if (right_x_inner < min_x)
            min_x = right_x_inner;
    }

    // 设置扇形光带脏矩形
    yps_cirle_gauge->dirty_rect.x = min_x;
    yps_cirle_gauge->dirty_rect.y = min_y;
    yps_cirle_gauge->dirty_rect.w = max_x - min_x;
    yps_cirle_gauge->dirty_rect.h = max_y - min_y;

    // 调试日志
    // printf("arc dirty_rect (%d, %d, %d, %d) start_angle: %f, end_angle: %f\n",
    //        yps_cirle_gauge->dirty_rect.x, yps_cirle_gauge->dirty_rect.y,
    //        yps_cirle_gauge->dirty_rect.w, yps_cirle_gauge->dirty_rect.h,
    //        start_angle, end_angle);

    // 计算指针区域的脏矩形
    if (tk_strlen(yps_cirle_gauge->pointer_image) > 0)
    {
        // 获取指针图片尺寸
        bitmap_t pointer_bmp;
        if (RET_OK == widget_load_image(widget, yps_cirle_gauge->pointer_image, &pointer_bmp))
        {
            float_t w = pointer_bmp.w;
            float_t h = pointer_bmp.h;

            // 计算指针旋转后的边界矩形
            float rotation = yps_cirle_gauge->angle * M_PI / 180.0;
            float last_rotation = yps_cirle_gauge->last_angle * M_PI / 180.0;

            // 计算指针图片的四个角点（相对于锚点）
            float corners[8];
            // 左上角
            corners[0] = -w / 2;
            +yps_cirle_gauge->pointer_offset_x;
            corners[1] = -h / 2 + yps_cirle_gauge->pointer_offset_y;
            // 右上角
            corners[2] = w / 2 + yps_cirle_gauge->pointer_offset_x;
            corners[3] = -h / 2 + yps_cirle_gauge->pointer_offset_y;
            // 右下角
            corners[4] = w / 2 + yps_cirle_gauge->pointer_offset_x;
            corners[5] = h / 2 + yps_cirle_gauge->pointer_offset_y;
            // 左下角
            corners[6] = -w / 2 + yps_cirle_gauge->pointer_offset_x;
            corners[7] = h / 2 + yps_cirle_gauge->pointer_offset_y;

            // 计算旋转后的坐标
            float rotated_corners[8];
            float last_rotated_corners[8];

            for (int i = 0; i < 8; i += 2)
            {
                // 当前角度旋转后的坐标
                rotated_corners[i] = corners[i] * cos(rotation) - corners[i + 1] * sin(rotation) + anchor_x;
                rotated_corners[i + 1] = corners[i] * sin(rotation) + corners[i + 1] * cos(rotation) + anchor_y;

                // 上一次角度旋转后的坐标
                last_rotated_corners[i] = corners[i] * cos(last_rotation) - corners[i + 1] * sin(last_rotation) + anchor_x;
                last_rotated_corners[i + 1] = corners[i] * sin(last_rotation) + corners[i + 1] * cos(last_rotation) + anchor_y;
            }

            // 找出两个位置的最小和最大x,y坐标
            float ptr_min_x = rotated_corners[0];
            float ptr_min_y = rotated_corners[1];
            float ptr_max_x = rotated_corners[0];
            float ptr_max_y = rotated_corners[1];

            for (int i = 0; i < 8; i += 2)
            {
                // 当前位置
                if (rotated_corners[i] < ptr_min_x)
                    ptr_min_x = rotated_corners[i];
                if (rotated_corners[i] > ptr_max_x)
                    ptr_max_x = rotated_corners[i];
                if (rotated_corners[i + 1] < ptr_min_y)
                    ptr_min_y = rotated_corners[i + 1];
                if (rotated_corners[i + 1] > ptr_max_y)
                    ptr_max_y = rotated_corners[i + 1];

                // 上一次位置
                if (last_rotated_corners[i] < ptr_min_x)
                    ptr_min_x = last_rotated_corners[i];
                if (last_rotated_corners[i] > ptr_max_x)
                    ptr_max_x = last_rotated_corners[i];
                if (last_rotated_corners[i + 1] < ptr_min_y)
                    ptr_min_y = last_rotated_corners[i + 1];
                if (last_rotated_corners[i + 1] > ptr_max_y)
                    ptr_max_y = last_rotated_corners[i + 1];
            }

            // 设置指针脏矩形
            yps_cirle_gauge->pointer_dirty_rect.x = ptr_min_x;
            yps_cirle_gauge->pointer_dirty_rect.y = ptr_min_y;
            yps_cirle_gauge->pointer_dirty_rect.w = ptr_max_x - ptr_min_x;
            yps_cirle_gauge->pointer_dirty_rect.h = ptr_max_y - ptr_min_y;
        }
        else
        {
            // 如果无法加载图片，设置一个默认的指针脏矩形
            yps_cirle_gauge->pointer_dirty_rect.x = anchor_x - 5;
            yps_cirle_gauge->pointer_dirty_rect.y = anchor_y - r1 - 5;
            yps_cirle_gauge->pointer_dirty_rect.w = 10;
            yps_cirle_gauge->pointer_dirty_rect.h = r1 + 10;
        }
    }
    else
    {
        // 如果没有指针图片，设置一个默认的指针脏矩形
        yps_cirle_gauge->pointer_dirty_rect.x = anchor_x - 10;
        yps_cirle_gauge->pointer_dirty_rect.y = anchor_y - r1 - 10;
        yps_cirle_gauge->pointer_dirty_rect.w = 20;
        yps_cirle_gauge->pointer_dirty_rect.h = r1 + 20;
    }

    // 合并两个脏矩形
    rect_t total_dirty_rect = merge_rects(yps_cirle_gauge->dirty_rect, yps_cirle_gauge->pointer_dirty_rect);

    total_dirty_rect.x += widget->x;
    total_dirty_rect.y += widget->y;

    // 使用合并后的脏矩形进行重绘
    return widget_invalidate_force(widget->parent, &total_dirty_rect);

    // 调试脏矩形 是直接刷新整个控件 保证脏矩形框可以正常绘制出来
    // return widget_invalidate_force(widget->parent, NULL);
}

static ret_t yps_cirle_gauge_on_event(widget_t *widget, event_t *e)
{
    yps_cirle_gauge_t *yps_cirle_gauge = YPS_CIRLE_GAUGE(widget);
    return_value_if_fail(widget != NULL && yps_cirle_gauge != NULL, RET_BAD_PARAMS);

    switch (e->type)
    {
    case EVT_WINDOW_LOAD:
        yps_cirle_gauge->dirty_rect.x = widget->x;
        yps_cirle_gauge->dirty_rect.y = widget->y;
        yps_cirle_gauge->dirty_rect.w = widget->w;
        yps_cirle_gauge->dirty_rect.h = widget->h;

        // 在窗口加载时预加载旋转位图
        if (tk_strlen(yps_cirle_gauge->pointer_image) > 0)
        {
            // load_pre_rotated_bitmaps(yps_cirle_gauge);
        }
        break;

    case EVT_PROP_CHANGED:
    {
        // 当指针图片属性改变时重新加载预旋转位图
        const prop_change_event_t *evt = (const prop_change_event_t *)e;
        if (evt != NULL && evt->name != NULL &&
            (tk_str_eq(evt->name, YPS_CIRLE_GAUGE_PROP_POINTER_IMAGE) ||
             tk_str_eq(evt->name, YPS_CIRLE_GAUGE_PROP_POINTER_IMAGE2)))
        {
            if (yps_cirle_gauge->pre_rotated_bitmaps_loaded)
            {
                // 重新加载预旋转位图
                // load_pre_rotated_bitmaps(yps_cirle_gauge);
            }
        }
        break;
    }
    }

    return RET_OK;
}

const char *s_yps_cirle_gauge_properties[] = {
    YPS_CIRLE_GAUGE_PROP_ANGLE,
    YPS_CIRLE_GAUGE_PROP_MIN_ANGLE,
    YPS_CIRLE_GAUGE_PROP_MAX_ANGLE,
    YPS_CIRLE_GAUGE_PROP_MIN_VALUE,
    YPS_CIRLE_GAUGE_PROP_MAX_VALUE,
    YPS_CIRLE_GAUGE_PROP_R1,
    YPS_CIRLE_GAUGE_PROP_R2,
    YPS_CIRLE_GAUGE_PROP_IMAGE1,
    YPS_CIRLE_GAUGE_PROP_IMAGE2,
    YPS_CIRLE_GAUGE_PROP_TOTAL_DEGREE,
    YPS_CIRLE_GAUGE_PROP_CRITICAL,
    YPS_CIRLE_GAUGE_PROP_ANCHOR_X,
    YPS_CIRLE_GAUGE_PROP_ANCHOR_Y,
    YPS_CIRLE_GAUGE_PROP_POINTER_IMAGE,
    YPS_CIRLE_GAUGE_PROP_POINTER_IMAGE2,
    YPS_CIRLE_GAUGE_PROP_POINTER_OFFSET_X,
    YPS_CIRLE_GAUGE_PROP_POINTER_OFFSET_Y,
    NULL};

TK_DECL_VTABLE(yps_cirle_gauge) = {.size = sizeof(yps_cirle_gauge_t),
                                   .type = WIDGET_TYPE_YPS_CIRLE_GAUGE,
                                   .clone_properties = s_yps_cirle_gauge_properties,
                                   .persistent_properties = s_yps_cirle_gauge_properties,
                                   .parent = TK_PARENT_VTABLE(widget),
                                   .create = yps_cirle_gauge_create,
                                   .on_paint_self = yps_cirle_gauge_on_paint_self,
                                   .invalidate = yps_cirle_gauge_invalidate,
                                   .set_prop = yps_cirle_gauge_set_prop,
                                   .get_prop = yps_cirle_gauge_get_prop,
                                   .on_event = yps_cirle_gauge_on_event,
                                   .on_destroy = yps_cirle_gauge_on_destroy};

widget_t *yps_cirle_gauge_create(widget_t *parent, xy_t x, xy_t y, wh_t w, wh_t h)
{
    widget_t *widget = widget_create(parent, TK_REF_VTABLE(yps_cirle_gauge), x, y, w, h);
    yps_cirle_gauge_t *yps_cirle_gauge = YPS_CIRLE_GAUGE(widget);
    return_value_if_fail(yps_cirle_gauge != NULL, NULL);

    yps_cirle_gauge->image1 = TKMEM_ALLOC(32);
    yps_cirle_gauge->image2 = TKMEM_ALLOC(32);
    yps_cirle_gauge->pointer_image = TKMEM_ALLOC(32);
    yps_cirle_gauge->pointer_image2 = TKMEM_ALLOC(32);

    memset(yps_cirle_gauge->image1, '\0', 32);
    memset(yps_cirle_gauge->image2, '\0', 32);
    strcpy(yps_cirle_gauge->pointer_image, "");
    strcpy(yps_cirle_gauge->pointer_image2, "");

    yps_cirle_gauge->angle = 0;
    yps_cirle_gauge->min_angle = -180;
    yps_cirle_gauge->max_angle = 180;
    yps_cirle_gauge->min_value = 0;
    yps_cirle_gauge->max_value = 12000;
    yps_cirle_gauge->r1 = 0;
    yps_cirle_gauge->r2 = 0;
    yps_cirle_gauge->total_degree = 180;
    yps_cirle_gauge->critical = 10000;
    yps_cirle_gauge->value = yps_cirle_gauge->min_value;
    yps_cirle_gauge->pointer_offset_x = 0;
    yps_cirle_gauge->pointer_offset_y = 0;

    // 初始化预旋转位图相关变量
    yps_cirle_gauge->pre_rotated_bitmaps = NULL;
    yps_cirle_gauge->num_pre_rotated_bitmaps = 0;
    yps_cirle_gauge->pre_rotated_bitmaps_loaded = FALSE;

    return widget;
}

widget_t *yps_cirle_gauge_cast(widget_t *widget)
{
    return_value_if_fail(WIDGET_IS_INSTANCE_OF(widget, yps_cirle_gauge), NULL);

    return widget;
}
