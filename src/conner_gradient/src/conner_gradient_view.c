#include "conner_gradient_view.h"
#ifdef __SSE2__
#include <emmintrin.h>
#include "tkc/color_parser.h"
/* 获取指定相对坐标的角度 */
static inline float polar_lut_get_angle(const polar_lut_t *lut, int32_t rel_x, int32_t rel_y)
{
    // 计算绝对坐标（相对于查询表的原点）
    int32_t abs_x = rel_x + lut->center_x;
    int32_t abs_y = rel_y + lut->center_y;

    // 边界检查
    if (abs_x < 0 || abs_x >= lut->width || abs_y < 0 || abs_y >= lut->height)
    {
        // 越界时回退到实时计算
        return atan2f((float)rel_y, (float)rel_x);
    }

    // 从查询表中获取预计算的角度值
    uint32_t index = abs_y * lut->width + abs_x;
    return lut->angle_table[index];
}

/* 获取指定相对坐标的距离 */
static inline float polar_lut_get_distance(const polar_lut_t *lut, int32_t rel_x, int32_t rel_y)
{
    int32_t abs_x = rel_x + lut->center_x;
    int32_t abs_y = rel_y + lut->center_y;

    if (abs_x < 0 || abs_x >= lut->width || abs_y < 0 || abs_y >= lut->height)
    {
        // 越界时回退到实时计算
        return sqrtf(rel_x * rel_x + rel_y * rel_y);
    }

    uint32_t index = abs_y * lut->width + abs_x;
    return lut->distance_table[index];
}

static void process_pixels_simd(const polar_lut_t *lut, color_t *color_table,
                                int32_t start_x, int32_t start_y, int32_t radius)
{
    // 一次处理4个像素
    for (int32_t y = -radius; y <= radius; y++)
    {
        for (int32_t x = -radius; x <= radius; x += 4)
        {
            // 使用SIMD指令同时计算4个位置的距离
            __m128 dx = _mm_set_ps(x + 3 - lut->center_x, x + 2 - lut->center_x,
                                   x + 1 - lut->center_x, x - lut->center_x);
            __m128 dy = _mm_set_ps1(y - lut->center_y);

            __m128 distance_sq = _mm_add_ps(_mm_mul_ps(dx, dx), _mm_mul_ps(dy, dy));
            __m128 radius_sq = _mm_set_ps1(radius * radius);

            // 比较距离（在半径内为1，否则为0）
            __m128 mask = _mm_cmple_ps(distance_sq, radius_sq);

            // TODO 只处理在弧形内的像素...
        }
    }
}
#endif

/* 颜色插值 */
static color_t color_interpolate(color_t c1, color_t c2, float ratio)
{
    color_t result;
    result.rgba.r = c1.rgba.r + (uint8_t)((c2.rgba.r - c1.rgba.r) * ratio);
    result.rgba.g = c1.rgba.g + (uint8_t)((c2.rgba.g - c1.rgba.g) * ratio);
    result.rgba.b = c1.rgba.b + (uint8_t)((c2.rgba.b - c1.rgba.b) * ratio);
    result.rgba.a = c1.rgba.a + (uint8_t)((c2.rgba.a - c1.rgba.a) * ratio);
    return result;
}

static polar_lut_t *polar_lut_create(int32_t max_width, int32_t max_height)
{
    polar_lut_t *lut = (polar_lut_t *)calloc(1, sizeof(polar_lut_t));
    lut->width = max_width;
    lut->height = max_height;
    lut->center_x = max_width / 2;
    lut->center_y = max_height / 2;

    // 分配内存
    size_t table_size = max_width * max_height * sizeof(float);
    lut->angle_table = (float *)malloc(table_size);
    lut->distance_table = (float *)malloc(table_size);

    // 预计算所有坐标的极坐标值
    for (int32_t y = 0; y < max_height; y++)
    {
        for (int32_t x = 0; x < max_width; x++)
        {
            int32_t idx = y * max_width + x;
            float dx = (float)(x - lut->center_x);
            float dy = (float)(y - lut->center_y);

            // 预计算角度和距离
            lut->angle_table[idx] = atan2f(dy, dx); // [-π, π]
            lut->distance_table[idx] = sqrtf(dx * dx + dy * dy);
        }
    }

    return lut;
}

static ret_t build_color_table(int32_t max_width, int32_t max_height, arc_gradient_renderer_t *renderer,
                               const color_t *points, uint32_t point_count)
{
    if (renderer->color_table == NULL)
    {
        renderer->lut = polar_lut_create(max_width, max_height);
        renderer->color_table_size = 360; // 1度精度
        renderer->color_table = (color_t *)malloc(renderer->color_table_size * sizeof(color_t));
    }

    // 预计算所有角度的颜色值
    for (uint32_t i = 0; i < renderer->color_table_size; i++)
    {
        float angle_ratio = (float)i / renderer->color_table_size; // [0, 1)
        renderer->color_table[i] = color_interpolate(points[0], points[1], angle_ratio);
    }

    renderer->cache_valid = TRUE;
    return RET_OK;
}

/* 加速的弧形渐变绘制 */
static ret_t draw_arc_gradient_fast(canvas_t *c, int32_t cx, int32_t cy,
                                    int32_t radius, arc_gradient_renderer_t *renderer)
{
    polar_lut_t *lut = renderer->lut;
    int32_t start_x = cx - lut->center_x;
    int32_t start_y = cy - lut->center_y;

    // 遍历弧形区域内的像素
    for (int32_t rel_y = -radius; rel_y <= radius; rel_y++)
    {
        for (int32_t rel_x = -radius; rel_x <= radius; rel_x++)
        {
            float distance = polar_lut_get_distance(lut, rel_x, rel_y);

            // 检查是否在弧形范围内
            if (distance <= radius)
            {
                // 使用查询表获取角度（避免实时atan2计算）
                float angle = polar_lut_get_angle(lut, rel_x, rel_y);

                // 将角度归一化到 [0, 2π)
                if (angle < 0)
                    angle += 2 * M_PI;

                // 查找预计算的颜色
                uint32_t angle_index = (uint32_t)((angle / (2 * M_PI)) * renderer->color_table_size);
                color_t pixel_color = renderer->color_table[angle_index % renderer->color_table_size];

                // 绘制像素
                canvas_set_fill_color(c, pixel_color);
                canvas_fill_rect(c, start_x + rel_x, start_y + rel_y, 1, 1);
            }
        }
    }

    return RET_OK;
}

/* 更新颜色表（彩虹渐变示例） */
static ret_t arc_gradient_renderer_update_colors(arc_gradient_renderer_t *renderer)
{
    return_value_if_fail(renderer != NULL && renderer->color_table != NULL, RET_BAD_PARAMS);

    for (uint32_t i = 0; i < renderer->color_table_size; i++)
    {
        float ratio = (float)i / renderer->color_table_size;
        /* 创建彩虹色：红->黄->绿->青->蓝->紫->红 */
        if (ratio < 0.166f)
        {
            /* 红->黄 */
            float r = 1.0f;
            float g = ratio / 0.166f;
            float b = 0.0f;
            renderer->color_table[i] = color_init((uint8_t)(r * 255), (uint8_t)(g * 255), (uint8_t)(b * 255), 255);
        }
        else if (ratio < 0.333f)
        {
            /* 黄->绿 */
            float r = 1.0f - (ratio - 0.166f) / 0.167f;
            float g = 1.0f;
            float b = 0.0f;
            renderer->color_table[i] = color_init((uint8_t)(r * 255), (uint8_t)(g * 255), (uint8_t)(b * 255), 255);
        }
        else if (ratio < 0.5f)
        {
            /* 绿->青 */
            float r = 0.0f;
            float g = 1.0f;
            float b = (ratio - 0.333f) / 0.167f;
            renderer->color_table[i] = color_init((uint8_t)(r * 255), (uint8_t)(g * 255), (uint8_t)(b * 255), 255);
        }
        else if (ratio < 0.666f)
        {
            /* 青->蓝 */
            float r = 0.0f;
            float g = 1.0f - (ratio - 0.5f) / 0.166f;
            float b = 1.0f;
            renderer->color_table[i] = color_init((uint8_t)(r * 255), (uint8_t)(g * 255), (uint8_t)(b * 255), 255);
        }
        else if (ratio < 0.833f)
        {
            /* 蓝->紫 */
            float r = (ratio - 0.666f) / 0.167f;
            float g = 0.0f;
            float b = 1.0f;
            renderer->color_table[i] = color_init((uint8_t)(r * 255), (uint8_t)(g * 255), (uint8_t)(b * 255), 255);
        }
        else
        {
            /* 紫->红 */
            float r = 1.0f;
            float g = 0.0f;
            float b = 1.0f - (ratio - 0.833f) / 0.167f;
            renderer->color_table[i] = color_init((uint8_t)(r * 255), (uint8_t)(g * 255), (uint8_t)(b * 255), 255);
        }
    }

    renderer->cache_valid = FALSE;
    return RET_OK;
}

/* 绘制弧形渐变 */
static ret_t conner_gradient_view_on_paint_self(widget_t *widget, canvas_t *c)
{
    conner_gradient_view_t *view = CONNER_GRADIENT_VIEW(widget);
    return_value_if_fail(view != NULL, RET_BAD_PARAMS);

    int32_t cx = widget->w / 2;
    int32_t cy = widget->h / 2;
    int32_t radius = tk_min(cx, cy) - 2;
    float_t start_angle = view->start_angle;
    float_t end_angle = view->stop_angle;

    /* 根据方向调整角度 */
    if (view->ant_clock)
    {
        float_t temp = start_angle;
        start_angle = end_angle;
        end_angle = temp;
    }
    /* 根据当前值计算实际结束角度 */
    if (view->max > 0)
    {
        float_t progress = (float_t)view->current / (float_t)view->max;
        end_angle = start_angle + (end_angle - start_angle) * progress;
    }

    /* debug */
    if (0)
    {
        arc_gradient_renderer_update_colors(&(view->arc_gradient_renderer));
    }
    else
    {
        draw_arc_gradient_fast(c, cx, cy, radius, &view->arc_gradient_renderer);
    }
    return RET_OK;
}

/* 设置属性 */
static ret_t conner_gradient_view_set_prop(widget_t *widget, const char *name, const value_t *v)
{
    conner_gradient_view_t *view = CONNER_GRADIENT_VIEW(widget);
    return_value_if_fail(view != NULL, RET_BAD_PARAMS);

    if (tk_str_eq(name, CONNER_GRADIENT_VIEW_PROP_START_ANGLE))
    {
        view->start_angle = value_float(v);
        return RET_OK;
    }
    else if (tk_str_eq(name, CONNER_GRADIENT_VIEW_PROP_STOP_ANGLE))
    {
        view->stop_angle = value_float(v);
        return RET_OK;
    }
    else if (tk_str_eq(name, CONNER_GRADIENT_VIEW_PROP_ANT_CLOCK))
    {
        view->ant_clock = value_bool(v);
        return RET_OK;
    }
    else if (tk_str_eq(name, CONNER_GRADIENT_VIEW_PROP_MAX))
    {
        view->max = value_int(v);
        return RET_OK;
    }
    else if (tk_str_eq(name, CONNER_GRADIENT_VIEW_PROP_CURRENT))
    {
        view->current = value_int(v);
        return RET_OK;
    }
    else if (tk_str_eq(name, CONNER_GRADIENT_VIEW_PROP_START_COLOR))
    {
        view->start_color = color_parse(value_str(v));
        return RET_OK;
    }
    else if (tk_str_eq(name, CONNER_GRADIENT_VIEW_PROP_STOP_COLOR))
    {
        view->stop_color = color_parse(value_str(v));
        return RET_OK;
    }

    return RET_NOT_FOUND;
}

/* 获取属性 */
static ret_t conner_gradient_view_get_prop(widget_t *widget, const char *name, value_t *v)
{
    conner_gradient_view_t *view = CONNER_GRADIENT_VIEW(widget);
    return_value_if_fail(view != NULL, RET_BAD_PARAMS);

    if (tk_str_eq(name, CONNER_GRADIENT_VIEW_PROP_START_ANGLE))
    {
        value_set_float(v, view->start_angle);
        return RET_OK;
    }
    else if (tk_str_eq(name, CONNER_GRADIENT_VIEW_PROP_STOP_ANGLE))
    {
        value_set_float(v, view->stop_angle);
        return RET_OK;
    }
    else if (tk_str_eq(name, CONNER_GRADIENT_VIEW_PROP_ANT_CLOCK))
    {
        value_set_bool(v, view->ant_clock);
        return RET_OK;
    }
    else if (tk_str_eq(name, CONNER_GRADIENT_VIEW_PROP_MAX))
    {
        value_set_int(v, view->max);
        return RET_OK;
    }
    else if (tk_str_eq(name, CONNER_GRADIENT_VIEW_PROP_CURRENT))
    {
        value_set_int(v, view->current);
        return RET_OK;
    }
    else if (tk_str_eq(name, CONNER_GRADIENT_VIEW_PROP_START_COLOR))
    {
        value_set_int(v, view->start_color.color);
        return RET_OK;
    }
    else if (tk_str_eq(name, CONNER_GRADIENT_VIEW_PROP_STOP_COLOR))
    {
        value_set_int(v, view->stop_color.color);
        return RET_OK;
    }

    return RET_NOT_FOUND;
}

/* 销毁控件 */
static ret_t conner_gradient_view_on_destroy(widget_t *widget)
{
    conner_gradient_view_t *view = CONNER_GRADIENT_VIEW(widget);
    return_value_if_fail(view != NULL, RET_BAD_PARAMS);
    return RET_OK;
}

/* 控件虚表 */
static const widget_vtable_t s_conner_gradient_view_vtable = {
    .size = sizeof(conner_gradient_view_t),
    .type = WIDGET_TYPE_CONNER_GRADIENT_VIEW,
    .create = conner_gradient_view_create,
    .set_prop = conner_gradient_view_set_prop,
    .get_prop = conner_gradient_view_get_prop,
    .on_paint_self = conner_gradient_view_on_paint_self,
    .on_destroy = conner_gradient_view_on_destroy};

/* 创建控件 */
widget_t *conner_gradient_view_create(widget_t *parent, xy_t x, xy_t y, wh_t w, wh_t h)
{
    widget_t *widget = widget_create(parent, &s_conner_gradient_view_vtable, x, y, w, h);
    conner_gradient_view_t *view = CONNER_GRADIENT_VIEW(widget);
    return_value_if_fail(view != NULL, NULL);

    /* 初始化默认值 */
    view->start_angle = 0;
    view->stop_angle = 2 * M_PI;
    view->ant_clock = FALSE;
    view->max = 100;
    view->current = 50;

    /* 创建弧形渐变渲染器 */
    int32_t max_size = tk_max(w, h) * 2;
    arc_gradient_renderer_t *arc_gradien = &(view->arc_gradient_renderer);
    color_t colors[] = {color_init(256, 0, 0, 256), color_init(0, 0, 256, 256)};
    build_color_table(widget->w, widget->h, &view->arc_gradient_renderer, colors, 2);

    return widget;
}

/* 类型转换 */
widget_t *conner_gradient_view_cast(widget_t *widget)
{
    return_value_if_fail(widget != NULL && widget->vt == &s_conner_gradient_view_vtable, NULL);
    return widget;
}

/* 设置角度范围 */
ret_t conner_gradient_view_set_angles(widget_t *widget, float_t start_angle, float_t stop_angle)
{
    conner_gradient_view_t *view = CONNER_GRADIENT_VIEW(widget);
    return_value_if_fail(view != NULL, RET_BAD_PARAMS);

    view->start_angle = start_angle;
    view->stop_angle = stop_angle;

    widget_invalidate(widget, NULL);
    return RET_OK;
}

/* 设置方向 */
ret_t conner_gradient_view_set_direction(widget_t *widget, bool_t ant_clock)
{
    conner_gradient_view_t *view = CONNER_GRADIENT_VIEW(widget);
    return_value_if_fail(view != NULL, RET_BAD_PARAMS);

    view->ant_clock = ant_clock;
    widget_invalidate(widget, NULL);
    return RET_OK;
}

/* 设置数值范围 */
ret_t conner_gradient_view_set_range(widget_t *widget, int32_t max, int32_t current)
{
    conner_gradient_view_t *view = CONNER_GRADIENT_VIEW(widget);
    return_value_if_fail(view != NULL, RET_BAD_PARAMS);

    view->max = max;
    view->current = current;

    widget_invalidate(widget, NULL);
    return RET_OK;
}

/* 设置当前值 */
ret_t conner_gradient_view_set_current(widget_t *widget, int32_t current)
{
    conner_gradient_view_t *view = CONNER_GRADIENT_VIEW(widget);
    return_value_if_fail(view != NULL, RET_BAD_PARAMS);

    view->current = current;
    widget_invalidate(widget, NULL);
    return RET_OK;
}
