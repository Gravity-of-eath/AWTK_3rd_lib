#include "conner_gradient_view.h"
#include "tkc/color_parser.h"
#include "base/bitmap.h"
#include "tkc/utils.h"
#include <math.h>
#include <stdio.h>

/* 弧度 -> 定点角。已按 AWTK 坐标系（Y 轴向下）把 0 度对齐到与原实现相同的方位。 */
#define ARC_RAD_TO_UNITS ((float)(ARC_ANGLE_UNITS / (2 * M_PI)))

/* 由相对圆心的坐标算出定点角。建表和越界回退共用同一份实现，保证两条路径一致。 */
static inline uint16_t arc_angle_from_xy(float dx, float dy)
{
    float a = atan2f(dy, dx);

    /* 归一化到 [0, 2PI) */
    if (a < 0) a += 2 * M_PI;

    /* AWTK 坐标系 Y 轴向下，减去 90 度，与原实现保持一致 */
    a -= M_PI_2;
    if (a < 0) a += 2 * M_PI;

    /* 四舍五入到定点角；恰好等于一整圈时回绕到 0 */
    return (uint16_t)((int32_t)(a * ARC_RAD_TO_UNITS + 0.5f) & (ARC_ANGLE_UNITS - 1));
}

/* 获取指定相对坐标的定点角 */
static inline uint16_t polar_lut_get_angle(const polar_lut_t *lut, int32_t rel_x, int32_t rel_y)
{
    // 计算绝对坐标（相对于查询表的原点）
    int32_t abs_x = rel_x + lut->center_x;
    int32_t abs_y = rel_y + lut->center_y;

    // 边界检查
    if (abs_x < 0 || abs_x >= lut->width || abs_y < 0 || abs_y >= lut->height)
    {
        // 越界时回退到实时计算
        return arc_angle_from_xy((float)rel_x, (float)rel_y);
    }

    // 从查询表中获取预计算的角度值
    uint32_t index = (uint32_t)abs_y * lut->width + abs_x;
    return lut->angle_table[index];
}

/* 注：这里原本有一份注释掉的 draw_arc_gradient_neon。
 * 它并没有真正向量化（中间用 vgetq_lane 逐个查表和绘制），而且引用了已被移除的
 * distance_table，因此删除。真正可行的 NEON 化建立在下面的定点角内核之上：
 * 一次 vld1q_u16 取 8 个角，vsub + vcle 得掩码，vshr 得色阶索引；
 * 两端点渐变还可以直接在向量里插值，连色表都不用查。
 */

/* 颜色插值 */
static color_t color_interpolate(color_t c1, color_t c2, float ratio)
{
    color_t result;
    // 确保ratio在[0,1]范围内
    ratio = tk_clamp(ratio, 0.0f, 1.0f);
    
    result.rgba.r = (uint8_t)(c1.rgba.r + (float)(c2.rgba.r - c1.rgba.r) * ratio);
    result.rgba.g = (uint8_t)(c1.rgba.g + (float)(c2.rgba.g - c1.rgba.g) * ratio);
    result.rgba.b = (uint8_t)(c1.rgba.b + (float)(c2.rgba.b - c1.rgba.b) * ratio);
    result.rgba.a = (uint8_t)(c1.rgba.a + (float)(c2.rgba.a - c1.rgba.a) * ratio);
    return result;
}

static polar_lut_t *polar_lut_create(int32_t max_width, int32_t max_height)
{
    polar_lut_t *lut = NULL;
    int32_t x = 0, y = 0;

    return_value_if_fail(max_width > 0 && max_height > 0, NULL);

    lut = (polar_lut_t *)TKMEM_ZALLOC(polar_lut_t);
    return_value_if_fail(lut != NULL, NULL);

    lut->width = max_width;
    lut->height = max_height;
    lut->center_x = max_width / 2;
    lut->center_y = max_height / 2;

    /* 只需一张 uint16 角度表：距离判定已改为逐行的整数区间求解 */
    lut->angle_table =
        (uint16_t *)TKMEM_ALLOC((size_t)max_width * max_height * sizeof(uint16_t));
    if (lut->angle_table == NULL) {
        TKMEM_FREE(lut);
        return NULL;
    }

    /* 预计算所有坐标的定点角（归一化与坐标系旋转都在这里烘进表里） */
    for (y = 0; y < max_height; y++)
    {
        uint16_t *row = lut->angle_table + (size_t)y * max_width;
        float dy = (float)(y - lut->center_y);

        for (x = 0; x < max_width; x++)
        {
            row[x] = arc_angle_from_xy((float)(x - lut->center_x), dy);
        }
    }

    return lut;
}

static ret_t build_color_table(arc_gradient_renderer_t *renderer, 
                               color_t start_color, color_t stop_color)
{
    // 初始化渲染器
    if (renderer->color_table == NULL)
    {
        renderer->color_table_size = 360; // 1度精度
        renderer->color_table = (color_t *)TKMEM_ALLOC(renderer->color_table_size * sizeof(color_t));
        return_value_if_fail(renderer->color_table != NULL, RET_OOM);
    }

    // 预计算所有角度的颜色值
    for (uint32_t i = 0; i < renderer->color_table_size; i++)
    {
        /* 分母用 size-1：原来除以 size，最大只能到 359/360，永远取不到 stop_color */
        float angle_ratio = (float)i / (renderer->color_table_size - 1); // [0, 1]
        renderer->color_table[i] = color_interpolate(start_color, stop_color, angle_ratio);
    }

    renderer->cache_valid = TRUE;
    return RET_OK;
}

/* 只准备颜色表。极坐标表交给 arc_gradient_renderer_ensure_lut 惰性创建：
 * 从 XML 创建控件时，widget 的宽高可能在 create 之后才确定。 */
static ret_t arc_gradient_renderer_init(arc_gradient_renderer_t *renderer,
                                        color_t start_color, color_t stop_color)
{
    return_value_if_fail(renderer != NULL, RET_BAD_PARAMS);

    renderer->lut = NULL;

    return build_color_table(renderer, start_color, stop_color);
}

/* 确保极坐标表与当前尺寸匹配；尺寸变化时重建。
 *
 * 原来只在 create() 里按初始宽高建一次表。控件之后被 layout 放大的话，
 * 查表会全面越界并退化成逐像素实时 atan2f，反而比不用表还慢。 */
static ret_t arc_gradient_renderer_ensure_lut(arc_gradient_renderer_t *renderer,
                                              int32_t width, int32_t height)
{
    return_value_if_fail(renderer != NULL, RET_BAD_PARAMS);

    if (renderer->lut != NULL &&
        renderer->lut->width == width && renderer->lut->height == height) {
        return RET_OK;
    }

    if (renderer->lut != NULL) {
        TKMEM_FREE(renderer->lut->angle_table);
        TKMEM_FREE(renderer->lut);
        renderer->lut = NULL;
    }

    renderer->lut = polar_lut_create(width, height);

    return renderer->lut != NULL ? RET_OK : RET_FAIL;
}

static ret_t arc_gradient_renderer_deinit(arc_gradient_renderer_t *renderer)
{
    return_value_if_fail(renderer != NULL, RET_BAD_PARAMS);
    
    if (renderer->lut) {
        TKMEM_FREE(renderer->lut->angle_table);
        TKMEM_FREE(renderer->lut);
        renderer->lut = NULL;
    }
    
    if (renderer->color_table) {
        TKMEM_FREE(renderer->color_table);
        renderer->color_table = NULL;
    }
    
    renderer->color_table_size = 0;
    renderer->cache_valid = FALSE;
    
    return RET_OK;
}

/*
 * 弧形渐变绘制。
 *
 * 相比最初的实现做了两处纯性能改造，像素输出结果保持不变：
 *
 * 1) 逐行区间扫描：不再遍历整个 (2r+1)^2 外接正方形再用距离判断丢弃，
 *    而是按行解析求出 x 区间。距离判定 sqrt(x^2+y^2) <= R 与整数判定
 *    x^2+y^2 <= R^2 严格等价，因此可以直接由 R^2 - y^2 求出该行半宽。
 *    圆外的角落(约 21.5% 面积)和内环空洞不再进入循环，
 *    distance_table 在绘制路径上也不再被访问。
 *
 * 2) 水平游程合并：同一扫描线上相邻像素的色阶索引大量相同，
 *    攒够一段再用一次 canvas_fill_rect(x0, y, len, 1) 输出，
 *    取代原来每像素一次 canvas_set_fill_color + canvas_fill_rect(...,1,1)。
 *
 * 另外把循环不变量(角度归一化、方向判定、angle_range)提到了循环外。
 * 原实现中 ant_clock 的 true/false 两个分支代码完全相同，这里合并为一份；
 * 二者真正的差异只在 angle_range 的取值上，此处按原样保留。
 */

/* 求满足 s*s <= v 的最大非负整数 s */
static inline int32_t arc_isqrt_floor(int32_t v)
{
    if (v <= 0) return 0;

    int32_t s = (int32_t)sqrtf((float)v);
    while (s > 0 && s * s > v) s--;
    while ((s + 1) * (s + 1) <= v) s++;

    return s;
}

/* 一条扫描线上绘制所需的、与像素无关的上下文。全部是整数量。
 *
 * 输出有两条路径：pixels 非空时直接往 RGBA8888 缓冲里写（缓存位图），
 * 否则退回到 canvas_fill_rect。直写省掉了每段游程一次的 canvas 调用，
 * 而 canvas 调用的固定开销正是实测中的主要瓶颈。 */
typedef struct _arc_span_ctx_t {
    canvas_t *c;         /* 回退路径 */
    uint8_t *pixels;     /* 直写路径：RGBA8888 缓冲首地址，NULL 表示走 canvas */
    uint32_t stride;     /* 直写路径：每行字节数 */
    int32_t dst_w;       /* 直写路径：缓冲宽高，用于裁剪 */
    int32_t dst_h;
    int32_t cx;
    int32_t cy;
    const polar_lut_t *lut;
    const color_t *color_table;
    uint32_t color_index_max;  /* color_table_size - 1 */
    uint16_t start_u;          /* 起始角（定点） */
    uint32_t span_u;           /* 弧的角度跨度（定点），用于判定像素是否在弧内 */
    uint32_t range_u;          /* 渐变分母（定点）。逆时针时与 span_u 不同 */
} arc_span_ctx_t;

/* 求某像素的色阶索引；不在弧内返回 -1。
 *
 * 关键点是这里的无符号回绕：d = (uint16)(angle - start) 天然处理了弧跨越 0 度的
 * 情况，一次无符号比较 d <= span 就替代了原来那一整组 if/else 分支，
 * 并且整条路径没有浮点运算、没有除法。
 */
static inline int32_t arc_color_index(const arc_span_ctx_t *ctx, uint16_t angle_u)
{
    uint32_t d = (uint16_t)(angle_u - ctx->start_u);
    uint32_t idx;

    if (d > ctx->span_u) return -1;

    /* 对应原实现里 position_ratio 被 clamp 到 1.0 的分支 */
    if (d > ctx->range_u) d = ctx->range_u;

    /* 精确的 floor(d / range * color_index_max)。
     * 这里刻意用整数除法而不是预计算的定点倒数：倒数的 16 位小数不够，
     * 会让色阶系统性偏低 1，实测约 33% 的像素受影响。
     * d <= ARC_ANGLE_UNITS 且 color_index_max 通常为 359，乘积不会溢出 uint32。 */
    idx = (d * ctx->color_index_max) / ctx->range_u;
    if (idx > ctx->color_index_max) idx = ctx->color_index_max;

    return (int32_t)idx;
}

/* 输出一段同色游程 [x0, x1]（闭区间，相对圆心坐标） */
static inline void arc_flush_run(const arc_span_ctx_t *ctx, int32_t rel_y, int32_t x0,
                                 int32_t x1, int32_t idx)
{
    color_t col = ctx->color_table[idx];

    if (ctx->pixels == NULL) {
        canvas_set_fill_color(ctx->c, col);
        canvas_fill_rect(ctx->c, ctx->cx + x0, ctx->cy + rel_y, x1 + 1 - x0, 1);
        return;
    }

    {
        int32_t y = ctx->cy + rel_y;
        int32_t xs = ctx->cx + x0;
        int32_t xe = ctx->cx + x1;
        uint32_t *p = NULL;

        if (y < 0 || y >= ctx->dst_h) return;
        if (xs < 0) xs = 0;
        if (xe >= ctx->dst_w) xe = ctx->dst_w - 1;
        if (xs > xe) return;

        /* color_t 是 rgba_t{r,g,b,a} 与 uint32 的联合体，这 4 个字节的内存序
         * 与 BITMAP_FMT_RGBA8888 完全一致，所以可以整字写入（不依赖字节序）。 */
        p = (uint32_t *)(ctx->pixels + (size_t)y * ctx->stride) + xs;
        while (xs <= xe) {
            *p++ = col.color;
            xs++;
        }
    }
}

/* 绘制第 rel_y 行上 [x0, x1] 这一段（闭区间，相对圆心坐标） */
static void draw_arc_span(const arc_span_ctx_t *ctx, int32_t rel_y, int32_t x0, int32_t x1)
{
    const polar_lut_t *lut = ctx->lut;
    const uint16_t *row = NULL;
    int32_t abs_y = rel_y + lut->center_y;
    int32_t run_x0 = x0;
    int32_t run_idx = -1; /* -1 表示当前没有待输出的游程 */
    int32_t rel_x = 0;

    /* 整段都落在 LUT 内时取行首指针，省掉逐像素的越界判断和乘法 */
    if (abs_y >= 0 && abs_y < lut->height && (x0 + lut->center_x) >= 0 &&
        (x1 + lut->center_x) < lut->width) {
        row = lut->angle_table + (size_t)abs_y * lut->width + lut->center_x;
    }

    for (rel_x = x0; rel_x <= x1; rel_x++)
    {
        uint16_t angle_u = (row != NULL) ? row[rel_x] : polar_lut_get_angle(lut, rel_x, rel_y);
        int32_t idx = arc_color_index(ctx, angle_u);

        if (idx != run_idx) {
            /* 色阶变化（或离开弧区），输出上一段游程 */
            if (run_idx >= 0) {
                arc_flush_run(ctx, rel_y, run_x0, rel_x - 1, run_idx);
            }
            run_idx = idx;
            run_x0 = rel_x;
        }
    }

    if (run_idx >= 0) {
        arc_flush_run(ctx, rel_y, run_x0, x1, run_idx);
    }
}

/* 弧度 -> 定点角跨度（不回绕，允许取到一整圈 ARC_ANGLE_UNITS） */
static inline uint32_t arc_span_to_units(float rad)
{
    int32_t u;

    if (rad <= 0) return 0;

    u = (int32_t)(rad * ARC_RAD_TO_UNITS + 0.5f);
    if (u < 0) u = 0;
    if (u > ARC_ANGLE_UNITS) u = ARC_ANGLE_UNITS;

    return (uint32_t)u;
}

/* 绘制弧形渐变。target 描述输出目标（canvas 或 RGBA8888 缓冲）。 */
static ret_t draw_arc_gradient(const arc_span_ctx_t *target, int32_t cx, int32_t cy,
                               int32_t radius, float start_angle, float end_angle,
                               bool_t ant_clock, arc_gradient_renderer_t *renderer,
                               float full_ratio)
{
    arc_span_ctx_t ctx;
    float angle_range = 0;
    float arc_span = 0;
    bool_t start_le_end = FALSE;
    int32_t inner_radius = 0;
    int32_t r2 = 0;
    int32_t ri2 = 0;
    int32_t rel_y = 0;
    bool_t full_circle = FALSE;

    return_value_if_fail(target != NULL && renderer != NULL && renderer->lut != NULL,
                         RET_BAD_PARAMS);
    return_value_if_fail(target->c != NULL || target->pixels != NULL, RET_BAD_PARAMS);
    return_value_if_fail(renderer->color_table != NULL && renderer->color_table_size > 1,
                         RET_BAD_PARAMS);

    if (radius <= 0) return RET_OK;

    /* 请求的扫角达到或超过一整圈时按整圆处理。
     * 必须在归一化之前判断：归一化会把 360 度扫角压成 0，原实现下
     * stop_angle=360 只会画出一条发丝般的残留细线，而不是整个圆环。 */
    full_circle = (end_angle - start_angle >= (float)(2 * M_PI)) ||
                  (start_angle - end_angle >= (float)(2 * M_PI));

    /* 确保角度在合理范围内。
     * 这里刻意保留原始的 while 写法而不用 fmodf：float 的 2π(6.2831855f) 比
     * double 的 2π 大约 1.7e-7，两种写法在 stop=360 这类边界上结果不同，
     * 会改变渲染结果。 */
    while (start_angle < 0) start_angle += 2 * M_PI;
    while (end_angle < 0) end_angle += 2 * M_PI;
    while (start_angle >= 2 * M_PI) start_angle -= 2 * M_PI;
    while (end_angle >= 2 * M_PI) end_angle -= 2 * M_PI;

    start_le_end = (start_angle <= end_angle);

    /* arc_span：判定像素是否落在弧内的角度跨度 */
    arc_span = start_le_end ? (end_angle - start_angle)
                            : ((float)(2 * M_PI) - start_angle + end_angle);

    /* angle_range：算渐变比例用的分母。逆时针时与 arc_span 不同，保持原有行为。 */
    if (ant_clock) {
        if (start_angle <= end_angle) {
            angle_range = (2 * M_PI - end_angle) + start_angle;
        } else {
            angle_range = start_angle - end_angle;
        }
    } else {
        if (end_angle <= start_angle) {
            angle_range = (2 * M_PI - start_angle) + end_angle;
        } else {
            angle_range = end_angle - start_angle;
        }
    }

    ctx = *target;
    ctx.cx = cx;
    ctx.cy = cy;
    ctx.lut = renderer->lut;
    ctx.color_table = renderer->color_table;
    ctx.color_index_max = renderer->color_table_size - 1;
    ctx.start_u = (uint16_t)((int32_t)(start_angle * ARC_RAD_TO_UNITS + 0.5f) &
                             (ARC_ANGLE_UNITS - 1));
    ctx.span_u = arc_span_to_units(arc_span);
    ctx.range_u = arc_span_to_units(angle_range);
    if (full_circle) {
        ctx.span_u = ARC_ANGLE_UNITS;
        ctx.range_u = ARC_ANGLE_UNITS;
    }
    if (ctx.range_u == 0) ctx.range_u = 1; /* 兜底，避免除零 */

    /* 计算内圆半径（用于绘制弧环） */
    inner_radius = (int32_t)(radius * (1.0f - full_ratio));
    if (inner_radius < 0) inner_radius = 0;

    r2 = radius * radius;
    ri2 = inner_radius * inner_radius;

    for (rel_y = -radius; rel_y <= radius; rel_y++)
    {
        int32_t yy = rel_y * rel_y;
        /* 外圆：满足 x*x <= r2 - yy 的最大 x */
        int32_t xo = arc_isqrt_floor(r2 - yy);
        /* 内圆空洞：满足 x*x < ri2 - yy 的最大 x，无空洞时为 -1 */
        int32_t hole = ri2 - yy;
        int32_t xi = (hole > 0) ? arc_isqrt_floor(hole - 1) : -1;

        if (xi >= xo) continue; /* 整行都被内圆挖空 */

        if (xi < 0) {
            draw_arc_span(&ctx, rel_y, -xo, xo);
        } else {
            draw_arc_span(&ctx, rel_y, -xo, -xi - 1);
            draw_arc_span(&ctx, rel_y, xi + 1, xo);
        }
    }

    return RET_OK;
}

/* 将角度转换为弧度 */
static inline float degrees_to_radians(float degrees) {
    return degrees * M_PI / 180.0f;
}

/* 绘制弧形渐变 */
/* 把当前参数换算成绘制所需的几何量并调用绘制内核。
 * cx/cy 用控件本地坐标，canvas 路径与缓存位图路径都适用。 */
static ret_t conner_gradient_view_render(conner_gradient_view_t *view,
                                         const arc_span_ctx_t *target)
{
    widget_t *widget = WIDGET(view);
    int32_t cx = widget->w / 2;
    int32_t cy = widget->h / 2;
    int32_t radius = tk_min(cx, cy) - 2;
    float_t start_angle = degrees_to_radians(view->start_angle);
    float_t stop_angle = degrees_to_radians(view->stop_angle);
    float_t end_angle = stop_angle;

    /* 根据当前值计算实际结束角度 */
    if (view->max > 0)
    {
        float_t progress = (float_t)view->current / (float_t)view->max;
        end_angle = start_angle + (stop_angle - start_angle) * progress;
    }

    return draw_arc_gradient(target, cx, cy, radius, start_angle, end_angle,
                             view->ant_clock, &view->arc_gradient_renderer, view->full_ratio);
}

/* 确保缓存位图存在且尺寸匹配。返回 NULL 表示不可用（调用方回退到直接绘制）。 */
static bitmap_t *conner_gradient_view_ensure_cache(conner_gradient_view_t *view,
                                                   int32_t w, int32_t h)
{
    if (view->cache_bitmap != NULL) {
        if ((int32_t)view->cache_bitmap->w == w && (int32_t)view->cache_bitmap->h == h) {
            return view->cache_bitmap;
        }
        /* 尺寸变了，丢弃重建 */
        bitmap_destroy(view->cache_bitmap);
        view->cache_bitmap = NULL;
    }

    /* RGBA8888：需要透明通道，弧形以外的区域不能盖住底下的内容 */
    view->cache_bitmap = bitmap_create_ex((uint32_t)w, (uint32_t)h, 0, BITMAP_FMT_RGBA8888);
    view->cache_dirty = TRUE;

    return view->cache_bitmap;
}

/* 把渲染结果写进缓存位图 */
static ret_t conner_gradient_view_fill_cache(conner_gradient_view_t *view, bitmap_t *bmp)
{
    arc_span_ctx_t target;
    uint8_t *data = bitmap_lock_buffer_for_write(bmp);
    uint32_t stride = bitmap_get_line_length(bmp);
    ret_t ret = RET_OK;

    return_value_if_fail(data != NULL && stride >= bmp->w * 4, RET_FAIL);

    memset(&target, 0, sizeof(target));
    target.pixels = data;
    target.stride = stride;
    target.dst_w = (int32_t)bmp->w;
    target.dst_h = (int32_t)bmp->h;

    /* 先整体清成全透明：弧形以外的区域必须不遮挡底下的内容 */
    memset(data, 0, (size_t)stride * bmp->h);

    ret = conner_gradient_view_render(view, &target);

    bitmap_unlock_buffer(bmp);

    return ret;
}

static ret_t conner_gradient_view_on_paint_self(widget_t *widget, canvas_t *c)
{
    conner_gradient_view_t *view = CONNER_GRADIENT_VIEW(widget);
    bitmap_t *bmp = NULL;

    return_value_if_fail(view != NULL, RET_BAD_PARAMS);

    if (widget->w <= 0 || widget->h <= 0) return RET_OK;

    /* 尺寸可能在 create 之后才由 XML/layout 确定，这里按当前尺寸惰性建表 */
    return_value_if_fail(
        arc_gradient_renderer_ensure_lut(&view->arc_gradient_renderer, widget->w, widget->h) ==
            RET_OK,
        RET_FAIL);

    if (view->cache_enable) {
        bmp = conner_gradient_view_ensure_cache(view, widget->w, widget->h);
    }

    if (bmp == NULL) {
        /* 关闭缓存或位图创建失败：直接画到 canvas 上，功能不受影响 */
        arc_span_ctx_t target;
        memset(&target, 0, sizeof(target));
        target.c = c;
        return conner_gradient_view_render(view, &target);
    }

    if (view->cache_dirty) {
        return_value_if_fail(conner_gradient_view_fill_cache(view, bmp) == RET_OK, RET_FAIL);
        view->cache_dirty = FALSE;
    }

    return canvas_draw_image_at(c, bmp, 0, 0);
}

/* 设置属性 */
static ret_t conner_gradient_view_set_prop(widget_t *widget, const char *name, const value_t *v)
{
    conner_gradient_view_t *view = CONNER_GRADIENT_VIEW(widget);
    return_value_if_fail(view != NULL, RET_BAD_PARAMS);

    if (tk_str_eq(name, CONNER_GRADIENT_VIEW_PROP_START_ANGLE))
    {
        view->start_angle = value_float(v);
        view->cache_dirty = TRUE;
        widget_invalidate(widget, NULL);
        return RET_OK;
    }
    else if (tk_str_eq(name, CONNER_GRADIENT_VIEW_PROP_STOP_ANGLE))
    {
        view->stop_angle = value_float(v);
        build_color_table(&view->arc_gradient_renderer, view->start_color, view->stop_color);
        view->cache_dirty = TRUE;
        widget_invalidate(widget, NULL);
        return RET_OK;
    }
    else if (tk_str_eq(name, CONNER_GRADIENT_VIEW_PROP_ANT_CLOCK))
    {
        view->ant_clock = value_bool(v);
        build_color_table(&view->arc_gradient_renderer, view->start_color, view->stop_color);
        view->cache_dirty = TRUE;
        widget_invalidate(widget, NULL);
        return RET_OK;
    }
    else if (tk_str_eq(name, CONNER_GRADIENT_VIEW_PROP_MAX))
    {
        view->max = value_int(v);
        view->cache_dirty = TRUE;
        widget_invalidate(widget, NULL);
        return RET_OK;
    }
    else if (tk_str_eq(name, CONNER_GRADIENT_VIEW_PROP_CURRENT))
    {
        view->current = value_int(v);
        view->cache_dirty = TRUE;
        widget_invalidate(widget, NULL);
        return RET_OK;
    }
    else if (tk_str_eq(name, CONNER_GRADIENT_VIEW_PROP_START_COLOR))
    {
        view->start_color = color_parse(value_str(v));
        // 重新初始化颜色表
        build_color_table(&view->arc_gradient_renderer, view->start_color, view->stop_color);
        view->cache_dirty = TRUE;
        widget_invalidate(widget, NULL);
        return RET_OK;
    }
    else if (tk_str_eq(name, CONNER_GRADIENT_VIEW_PROP_STOP_COLOR))
    {
        view->stop_color = color_parse(value_str(v));
        // 重新初始化颜色表
        build_color_table(&view->arc_gradient_renderer, view->start_color, view->stop_color);
        view->cache_dirty = TRUE;
        widget_invalidate(widget, NULL);
        return RET_OK;
    }
    else if (tk_str_eq(name, "full_ratio"))
    {
        view->full_ratio = value_float(v);
        view->cache_dirty = TRUE;
        widget_invalidate(widget, NULL);
        return RET_OK;
    }
    else if (tk_str_eq(name, CONNER_GRADIENT_VIEW_PROP_CACHE))
    {
        view->cache_enable = value_bool(v);
        view->cache_dirty = TRUE;
        widget_invalidate(widget, NULL);
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
        char color_str[16];
        snprintf(color_str, sizeof(color_str), "#%02X%02X%02X%02X",
                view->start_color.rgba.r, view->start_color.rgba.g,
                view->start_color.rgba.b, view->start_color.rgba.a);
        value_set_str(v, color_str);
        return RET_OK;
    }
    else if (tk_str_eq(name, CONNER_GRADIENT_VIEW_PROP_STOP_COLOR))
    {
        char color_str[16];
        snprintf(color_str, sizeof(color_str), "#%02X%02X%02X%02X",
                view->stop_color.rgba.r, view->stop_color.rgba.g,
                view->stop_color.rgba.b, view->stop_color.rgba.a);
        value_set_str(v, color_str);
        return RET_OK;
    }
    else if (tk_str_eq(name, "full_ratio"))
    {
        value_set_float(v, view->full_ratio);
        return RET_OK;
    }
    else if (tk_str_eq(name, CONNER_GRADIENT_VIEW_PROP_CACHE))
    {
        value_set_bool(v, view->cache_enable);
        return RET_OK;
    }

    return RET_NOT_FOUND;
}

/* 销毁控件 */
static ret_t conner_gradient_view_on_destroy(widget_t *widget)
{
    conner_gradient_view_t *view = CONNER_GRADIENT_VIEW(widget);
    return_value_if_fail(view != NULL, RET_BAD_PARAMS);
    
    if (view->cache_bitmap != NULL) {
        bitmap_destroy(view->cache_bitmap);
        view->cache_bitmap = NULL;
    }

    // 释放弧形渐变渲染器资源
    arc_gradient_renderer_deinit(&view->arc_gradient_renderer);
    
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
    view->stop_angle = 360.0f; /* 单位是度：on_paint_self 里还会做一次度->弧度 */
    view->ant_clock = FALSE;
    view->max = 100;
    view->current = 50;
    view->start_color = color_init(255, 0, 0, 255);    // 红色
    view->stop_color = color_init(0, 0, 255, 255);     // 蓝色
    view->full_ratio = 1.0f;  // 默认绘制整个扇形
    view->cache_bitmap = NULL;
    view->cache_dirty = TRUE;
    view->cache_enable = TRUE;

    /* 初始化弧形渐变渲染器 */
    ret_t ret = arc_gradient_renderer_init(&view->arc_gradient_renderer,
                                           view->start_color,
                                           view->stop_color);
    if (ret != RET_OK) {
        widget_destroy(widget);
        return NULL;
    }

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

    view->cache_dirty = TRUE;
    widget_invalidate(widget, NULL);
    return RET_OK;
}

/* 设置方向 */
ret_t conner_gradient_view_set_direction(widget_t *widget, bool_t ant_clock)
{
    conner_gradient_view_t *view = CONNER_GRADIENT_VIEW(widget);
    return_value_if_fail(view != NULL, RET_BAD_PARAMS);

    view->ant_clock = ant_clock;
    view->cache_dirty = TRUE;
    widget_invalidate(widget, NULL);
    return RET_OK;
}

/* 设置最大值 */
ret_t conner_gradient_view_set_max(widget_t *widget, int32_t max)
{
    conner_gradient_view_t *view = CONNER_GRADIENT_VIEW(widget);
    return_value_if_fail(view != NULL, RET_BAD_PARAMS);

    view->max = max;

    view->cache_dirty = TRUE;
    widget_invalidate(widget, NULL);
    return RET_OK;
}

/* 设置当前值 */
ret_t conner_gradient_view_set_current(widget_t *widget, int32_t current)
{
    conner_gradient_view_t *view = CONNER_GRADIENT_VIEW(widget);
    return_value_if_fail(view != NULL, RET_BAD_PARAMS);

    view->current = current;
    view->cache_dirty = TRUE;
    widget_invalidate(widget, NULL);
    return RET_OK;
}
