#include "yps_gl_view.h"
#include "tkc/mem.h"
#include "tkc/utils.h"
#include "tkc/timer_manager.h"
#include "base/vgcanvas.h"
#include "base/window_manager.h"

/* C wrappers implemented in ogre_awtk_app.cpp */
#include "ogre_awtk_app.hpp"

typedef struct _yps_gl_view_ext_t {
    void* ogre_app_ptr;
    uint32_t timer_id;
} yps_gl_view_ext_t;

#define YPS_GL_VIEW_EXT(widget) ((yps_gl_view_ext_t*)((yps_gl_view_t*)(widget) + 1))

/* ------------------------------------------------------------------ */
/*  Timer: drives continuous invalidation for animation rendering      */
/* ------------------------------------------------------------------ */

static ret_t on_invalidate_timer(const timer_info_t* info) {
    widget_invalidate_force(WIDGET(info->ctx), NULL);
    return RET_REPEAT;
}

/* ------------------------------------------------------------------ */
/*  Paint                                                              */
/* ------------------------------------------------------------------ */

static ret_t yps_gl_view_on_paint_self(widget_t* widget, canvas_t* c) {
    yps_gl_view_t* yps_gl_view = YPS_GL_VIEW(widget);
    yps_gl_view_ext_t* ext = YPS_GL_VIEW_EXT(widget);

    if (widget->w < 1 || widget->h < 1) return RET_OK;

    /* 1. 初始化 OGRE（首次绘制时，不依赖 FBO 状态） */
    if (!ext->ogre_app_ptr) {
        ext->ogre_app_ptr = ogre_awtk_init(
            yps_gl_view->scene_file, yps_gl_view->content_dir,
            widget->w, widget->h);
        if (!ext->ogre_app_ptr) {
            log_error("yps_gl_view: ogre_awtk_init failed\n");
            return RET_OK;
        }
    }

    /* 2. 创建 bitmap（两种模式都需要） */
    if (!yps_gl_view->bitmap) {
        yps_gl_view->bitmap = bitmap_create_ex(
            widget->w, widget->h, widget->w * 4, BITMAP_FMT_RGBA8888);
        if (!yps_gl_view->bitmap) {
            log_error("yps_gl_view: bitmap_create_ex failed\n");
            return RET_OK;
        }
        yps_gl_view->bitmap->flags = 0;
        yps_gl_view->bitmap->specific = 0;
        yps_gl_view->bitmap->specific_ctx = NULL;
        bitmap_set_dirty(yps_gl_view->bitmap, FALSE);
    }

    /* 3. 零拷贝模式：创建 AWTK FBO 并绑定到 bitmap（参考 test/triangle_fbo_to_awtk/main.c） */
    if (!yps_gl_view->use_readback && yps_gl_view->fbo.handle == NULL) {
        vgcanvas_t* vg = canvas_get_vgcanvas(c);
        if (vg != NULL) {
            if (vgcanvas_create_fbo(vg, widget->w, widget->h, TRUE, &(yps_gl_view->fbo)) == RET_OK) {
                fbo_to_img(&(yps_gl_view->fbo), yps_gl_view->bitmap);
                bitmap_set_dirty(yps_gl_view->bitmap, FALSE);
                log_debug("yps_gl_view: AWTK FBO created: id=%d offline_fbo=%d\n",
                          yps_gl_view->fbo.id, yps_gl_view->fbo.offline_fbo);
            } else {
                log_error("yps_gl_view: vgcanvas_create_fbo failed\n");
            }
        }
    }

    /* 4. OGRE 渲染一帧（渲染到 OGRE 自身的 offScreenTarget=fboTexture） */
    ogre_awtk_render_frame(ext->ogre_app_ptr);

    /* 5. 将渲染结果传输到 AWTK 可绘制的 bitmap */
    if (yps_gl_view->use_readback) {
        /*
         * 回读模式（方案 A）：glReadPixels 从 OGRE 纹理回读到 CPU 内存
         * 对齐 fbo_to_bitmap.md 方案 A 和 test/triangle_fbo_to_awtk/main.c
         */
        uint8_t* data = bitmap_lock_buffer_for_write(yps_gl_view->bitmap);
        if (data) {
            ogre_awtk_readback(ext->ogre_app_ptr, data, widget->w, widget->h);
            bitmap_unlock_buffer(yps_gl_view->bitmap);
            bitmap_set_dirty(yps_gl_view->bitmap, TRUE);
        }

        /* 清除 GPU 纹理标志，强制 AWTK 使用 CPU 路径采样 */
        uint32_t old_flags = yps_gl_view->bitmap->flags;
        yps_gl_view->bitmap->flags &= ~(BITMAP_FLAG_TEXTURE | BITMAP_FLAG_GPU_FBO_TEXTURE);

        rect_t src_rect = rect_init(0, 0, yps_gl_view->bitmap->w, yps_gl_view->bitmap->h);
        rect_t dst_rect = rect_init(0, 0, widget->w, widget->h);
        canvas_draw_image(c, yps_gl_view->bitmap, &src_rect, &dst_rect);

        yps_gl_view->bitmap->flags = old_flags;

    } else if (yps_gl_view->fbo.handle != NULL) {
        /*
         * 零拷贝模式（方案 B）：OGRE 纹理 → blit 到 AWTK FBO → GPU 直接采样
         * OGRE renderOneFrame 内部绑定自身 FBO，结果在 offscreen_tex_id 中。
         * 通过 blit（全屏四边形 + 纹理采样）将 OGRE 纹理复制到 AWTK 的 offline_fbo。
         */
        ogre_awtk_blit_to_fbo(ext->ogre_app_ptr,
                               yps_gl_view->fbo.offline_fbo,
                               widget->w, widget->h);

        /* bitmap_set_dirty(FALSE) 防止 AWTK 从 CPU 缓冲区重新加载纹理 */
        bitmap_set_dirty(yps_gl_view->bitmap, FALSE);

        rect_t src_rect = rect_init(0, 0, yps_gl_view->bitmap->w, yps_gl_view->bitmap->h);
        rect_t dst_rect = rect_init(0, 0, widget->w, widget->h);
        canvas_draw_image(c, yps_gl_view->bitmap, &src_rect, &dst_rect);
    }

    return RET_OK;
}

/* ------------------------------------------------------------------ */
/*  Destroy                                                            */
/* ------------------------------------------------------------------ */

static ret_t yps_gl_view_on_destroy(widget_t* widget) {
    yps_gl_view_t* yps_gl_view = YPS_GL_VIEW(widget);
    yps_gl_view_ext_t* ext = YPS_GL_VIEW_EXT(widget);

    /* 1. 先销毁 OGRE（释放 GL 上下文和资源） */
    if (ext->ogre_app_ptr) {
        ogre_awtk_deinit(ext->ogre_app_ptr);
        ext->ogre_app_ptr = NULL;
    }

    /* 2. 销毁 AWTK FBO（需要 vgcanvas，通过 window_manager 获取） */
    if (yps_gl_view->fbo.handle != NULL) {
        widget_t* wm = window_manager();
        if (wm != NULL) {
            canvas_t* canvas = widget_get_canvas(wm);
            if (canvas != NULL) {
                vgcanvas_t* vg = canvas_get_vgcanvas(canvas);
                if (vg != NULL) {
                    vgcanvas_destroy_fbo(vg, &(yps_gl_view->fbo));
                }
            }
        }
        memset(&(yps_gl_view->fbo), 0, sizeof(yps_gl_view->fbo));
    }

    /* 3. 销毁 bitmap */
    if (yps_gl_view->bitmap) {
        bitmap_destroy(yps_gl_view->bitmap);
        yps_gl_view->bitmap = NULL;
    }

    /* 4. 释放字符串 */
    TFREE(yps_gl_view->scene_file);
    TFREE(yps_gl_view->content_dir);

    /* 5. 释放 mode_lists */
    if (yps_gl_view->mode_lists) {
        int32_t i;
        for (i = 0; i < yps_gl_view->mode_count; i++) {
            TFREE(yps_gl_view->mode_lists[i]);
        }
        TFREE(yps_gl_view->mode_lists);
        yps_gl_view->mode_lists = NULL;
        yps_gl_view->mode_count = 0;
    }

    return RET_OK;
}

/* ------------------------------------------------------------------ */
/*  Properties                                                         */
/* ------------------------------------------------------------------ */

static ret_t yps_gl_view_get_prop(widget_t* widget, const char* name, value_t* v) {
    yps_gl_view_t* yps_gl_view = YPS_GL_VIEW(widget);

    if (tk_str_eq(name, YPS_GL_VIEW_PROP_SCENE_FILE)) {
        value_set_str(v, yps_gl_view->scene_file);
        return RET_OK;
    } else if (tk_str_eq(name, YPS_GL_VIEW_PROP_CONTENT_DIR)) {
        value_set_str(v, yps_gl_view->content_dir);
        return RET_OK;
    }

    return RET_NOT_FOUND;
}

static ret_t yps_gl_view_set_prop(widget_t* widget, const char* name, const value_t* v) {
    if (tk_str_eq(name, YPS_GL_VIEW_PROP_SCENE_FILE)) {
        return yps_gl_view_set_scene_file(widget, value_str(v));
    } else if (tk_str_eq(name, YPS_GL_VIEW_PROP_CONTENT_DIR)) {
        return yps_gl_view_set_content_dir(widget, value_str(v));
    }

    return RET_NOT_FOUND;
}

/* ------------------------------------------------------------------ */
/*  VTable & Create                                                    */
/* ------------------------------------------------------------------ */

static const widget_vtable_t s_yps_gl_view_vtable = {
    .size = sizeof(yps_gl_view_t) + sizeof(yps_gl_view_ext_t),
    .type = WIDGET_TYPE_YPS_GL_VIEW,
    .get_parent_vt = TK_GET_PARENT_VTABLE(widget),
    .create = yps_gl_view_create,
    .on_paint_self = yps_gl_view_on_paint_self,
    .on_destroy = yps_gl_view_on_destroy,
    .get_prop = yps_gl_view_get_prop,
    .set_prop = yps_gl_view_set_prop,
};

widget_t* yps_gl_view_create(widget_t* parent, xy_t x, xy_t y, wh_t w, wh_t h) {
    widget_t* widget = widget_create(parent, &s_yps_gl_view_vtable, x, y, w, h);
    yps_gl_view_t* yps_gl_view = YPS_GL_VIEW(widget);
    return_value_if_fail(yps_gl_view != NULL, NULL);

    yps_gl_view->use_readback = FALSE;
    yps_gl_view->bitmap = NULL;
    yps_gl_view->scene_file = NULL;
    yps_gl_view->mode_lists = NULL;
    yps_gl_view->mode_count = 0;
    yps_gl_view->content_dir = NULL;
    memset(&(yps_gl_view->fbo), 0, sizeof(yps_gl_view->fbo));

    yps_gl_view_ext_t* ext = YPS_GL_VIEW_EXT(widget);
    memset(ext, 0, sizeof(yps_gl_view_ext_t));

    /* 添加定时器驱动持续渲染（~30FPS），对齐 test/triangle_fbo_to_awtk/main.c */
    ext->timer_id = widget_add_timer(widget, on_invalidate_timer, 1000 / 30);

    return widget;
}

/* ------------------------------------------------------------------ */
/*  Public API                                                         */
/* ------------------------------------------------------------------ */

ret_t yps_gl_view_set_scene_file(widget_t* widget, const char* scene_file) {
    yps_gl_view_t* yps_gl_view = YPS_GL_VIEW(widget);
    return_value_if_fail(yps_gl_view != NULL, RET_BAD_PARAMS);

    TFREE(yps_gl_view->scene_file);
    yps_gl_view->scene_file = tk_strdup(scene_file);

    return RET_OK;
}

ret_t yps_gl_view_set_content_dir(widget_t* widget, const char* content_dir) {
    yps_gl_view_t* yps_gl_view = YPS_GL_VIEW(widget);
    return_value_if_fail(yps_gl_view != NULL, RET_BAD_PARAMS);

    TFREE(yps_gl_view->content_dir);
    yps_gl_view->content_dir = tk_strdup(content_dir);

    return RET_OK;
}

const char* yps_gl_view_get_content_dir(widget_t* widget) {
    yps_gl_view_t* yps_gl_view = YPS_GL_VIEW(widget);
    return_value_if_fail(yps_gl_view != NULL, NULL);

    return yps_gl_view->content_dir;
}

ret_t yps_gl_view_set_readback(widget_t* widget, bool_t use_readback) {
    yps_gl_view_t* yps_gl_view = YPS_GL_VIEW(widget);
    return_value_if_fail(yps_gl_view != NULL, RET_BAD_PARAMS);

    yps_gl_view->use_readback = use_readback;

    return RET_OK;
}

ret_t yps_gl_view_switch_scene(widget_t* widget, const char* scene_file) {
    return yps_gl_view_set_scene_file(widget, scene_file);
}

ret_t yps_gl_view_reload_scene(widget_t* widget) {
    yps_gl_view_t* yps_gl_view = YPS_GL_VIEW(widget);
    return yps_gl_view_set_scene_file(widget, yps_gl_view->scene_file);
}

ret_t yps_gl_view_unload_scene(widget_t* widget) {
    yps_gl_view_ext_t* ext = YPS_GL_VIEW_EXT(widget);
    if (ext->ogre_app_ptr) {
        ogre_awtk_deinit(ext->ogre_app_ptr);
        ext->ogre_app_ptr = NULL;
    }
    return RET_OK;
}

ret_t yps_gl_view_set_target_fps(widget_t* widget, uint32_t fps) {
    yps_gl_view_ext_t* ext = YPS_GL_VIEW_EXT(widget);
    return_value_if_fail(widget != NULL && fps > 0, RET_BAD_PARAMS);

    if (ext->timer_id != 0) {
        timer_remove(ext->timer_id);
        ext->timer_id = 0;
    }
    ext->timer_id = widget_add_timer(widget, on_invalidate_timer, 1000 / fps);

    return RET_OK;
}
