#include "yps_gl_view.h"
#include "tkc/mem.h"
#include "tkc/str.h"
#include "tkc/utils.h"
#include "tkc/timer_manager.h"
#include "base/vgcanvas.h"
#include "base/window_manager.h"

/* C wrappers implemented in ogre_awtk_app.cpp */
#include "ogre_awtk_app.hpp"

#define YPS_GL_VIEW_DEFAULT_FPS 30

typedef struct _yps_gl_view_ext_t {
    void* ogre_app_ptr;
    uint32_t timer_id;
    uint32_t target_fps;
    bool_t init_failed;   /* OGRE 初始化失败，不再每帧重试 */
    bool_t scene_dirty;   /* 场景文件/模型列表有变化，下一帧重新加载 */
    bool_t surface_dirty; /* bitmap/FBO 需要重建 */
    wh_t tex_w;           /* 当前离屏资源的尺寸 */
    wh_t tex_h;
} yps_gl_view_ext_t;

/* 参数必须是已校验过的 yps_gl_view_t*，避免 YPS_GL_VIEW() 里的 cast 被重复调用 */
#define YPS_GL_VIEW_EXT(gl_view) ((yps_gl_view_ext_t*)((yps_gl_view_t*)(gl_view) + 1))

/* ------------------------------------------------------------------ */
/*  Timer: drives continuous invalidation for animation rendering      */
/* ------------------------------------------------------------------ */

static ret_t on_invalidate_timer(const timer_info_t* info) {
    widget_invalidate_force(WIDGET(info->ctx), NULL);
    return RET_REPEAT;
}

/* ------------------------------------------------------------------ */
/*  Internal helpers                                                   */
/* ------------------------------------------------------------------ */

static ret_t yps_gl_view_clear_models(yps_gl_view_t* gl_view) {
    if (gl_view->mode_lists != NULL) {
        int32_t i = 0;
        for (i = 0; i < gl_view->mode_count; i++) {
            TKMEM_FREE(gl_view->mode_lists[i]);
        }
        TKMEM_FREE(gl_view->mode_lists);
    }
    gl_view->mode_lists = NULL;
    gl_view->mode_count = 0;

    return RET_OK;
}

/* 释放离屏绘制资源（bitmap + AWTK FBO）。vg 为 NULL 时只丢弃 FBO 记录。 */
static ret_t yps_gl_view_release_surface(yps_gl_view_t* gl_view, vgcanvas_t* vg) {
    if (gl_view->fbo.handle != NULL) {
        if (vg != NULL) {
            vgcanvas_destroy_fbo(vg, &(gl_view->fbo));
        } else {
            log_warn("yps_gl_view: no vgcanvas, FBO %d leaked\n", gl_view->fbo.id);
        }
        memset(&(gl_view->fbo), 0, sizeof(gl_view->fbo));
    }

    if (gl_view->bitmap != NULL) {
        bitmap_destroy(gl_view->bitmap);
        gl_view->bitmap = NULL;
    }

    return RET_OK;
}

/* 就地切分 "a.mesh;b.mesh, c.mesh"，返回 token 个数，最多写 max 个到 out */
static uint32_t yps_gl_view_split_models(char* s, char** out, uint32_t max) {
    uint32_t n = 0;
    char* p = s;

    while (*p != '\0') {
        char* start = NULL;
        char* end = NULL;

        while (*p == ' ' || *p == '\t' || *p == ';' || *p == ',') p++;
        if (*p == '\0') break;

        start = p;
        while (*p != '\0' && *p != ';' && *p != ',') p++;
        end = p;
        if (*p != '\0') {
            *p = '\0';
            p++;
        }
        while (end > start && (end[-1] == ' ' || end[-1] == '\t')) {
            end--;
            *end = '\0';
        }

        if (*start != '\0') {
            if (out != NULL && n < max) out[n] = start;
            n++;
        }
    }

    return n;
}

/* ------------------------------------------------------------------ */
/*  Paint                                                              */
/* ------------------------------------------------------------------ */

static ret_t yps_gl_view_on_paint_self(widget_t* widget, canvas_t* c) {
    yps_gl_view_t* gl_view = YPS_GL_VIEW(widget);
    yps_gl_view_ext_t* ext = NULL;
    vgcanvas_t* vg = NULL;
    return_value_if_fail(gl_view != NULL && c != NULL, RET_BAD_PARAMS);

    ext = YPS_GL_VIEW_EXT(gl_view);
    vg = canvas_get_vgcanvas(c);

    if (widget->w < 1 || widget->h < 1) return RET_OK;

    /* 1. 初始化 OGRE（首次绘制时，不依赖 FBO 状态） */
    if (ext->ogre_app_ptr == NULL) {
        if (ext->init_failed) return RET_OK;

        ext->ogre_app_ptr = ogre_awtk_init(gl_view->content_dir, widget->w, widget->h);
        if (ext->ogre_app_ptr == NULL) {
            log_error("yps_gl_view: ogre_awtk_init failed\n");
            ext->init_failed = TRUE;
            return RET_OK;
        }
        ext->tex_w = widget->w;
        ext->tex_h = widget->h;
        ext->scene_dirty = TRUE;
        ext->surface_dirty = TRUE;
    }

    /* 2. 尺寸变化：同步 OGRE 离屏目标并重建 bitmap/FBO */
    if (ext->tex_w != widget->w || ext->tex_h != widget->h) {
        ogre_awtk_resize(ext->ogre_app_ptr, widget->w, widget->h);
        ext->tex_w = widget->w;
        ext->tex_h = widget->h;
        ext->surface_dirty = TRUE;
    }

    if (ext->surface_dirty) {
        yps_gl_view_release_surface(gl_view, vg);
        ext->surface_dirty = FALSE;
    }

    /* 3. 场景内容加载（scene_file 优先于模型列表） */
    if (ext->scene_dirty) {
        if (gl_view->scene_file != NULL && gl_view->scene_file[0] != '\0') {
            ogre_awtk_load_scene(ext->ogre_app_ptr, gl_view->scene_file);
        } else if (gl_view->mode_lists != NULL && gl_view->mode_count > 0) {
            ogre_awtk_load_models(ext->ogre_app_ptr,
                                  (const char* const*)gl_view->mode_lists,
                                  gl_view->mode_count);
        } else {
            ogre_awtk_clear_scene(ext->ogre_app_ptr);
        }
        ext->scene_dirty = FALSE;
    }

    /* 4. 创建 bitmap（两种模式都需要） */
    if (gl_view->bitmap == NULL) {
        gl_view->bitmap = bitmap_create_ex(
            widget->w, widget->h, widget->w * 4, BITMAP_FMT_RGBA8888);
        if (gl_view->bitmap == NULL) {
            log_error("yps_gl_view: bitmap_create_ex failed\n");
            return RET_OK;
        }
        gl_view->bitmap->flags = 0;
        gl_view->bitmap->specific = 0;
        gl_view->bitmap->specific_ctx = NULL;
        bitmap_set_dirty(gl_view->bitmap, FALSE);
    }

    /* 5. 零拷贝模式：创建 AWTK FBO 并绑定到 bitmap */
    if (!gl_view->use_readback && gl_view->fbo.handle == NULL && vg != NULL) {
        if (vgcanvas_create_fbo(vg, widget->w, widget->h, TRUE, &(gl_view->fbo)) == RET_OK) {
            fbo_to_img(&(gl_view->fbo), gl_view->bitmap);
            bitmap_set_dirty(gl_view->bitmap, FALSE);
            log_debug("yps_gl_view: AWTK FBO created: id=%d offline_fbo=%d\n",
                      gl_view->fbo.id, gl_view->fbo.offline_fbo);
        } else {
            log_error("yps_gl_view: vgcanvas_create_fbo failed\n");
        }
    }

    /* 6. OGRE 渲染一帧（渲染到 OGRE 自身的 offScreenTarget=fboTexture） */
    ogre_awtk_render_frame(ext->ogre_app_ptr);

    /* 7. 将渲染结果传输到 AWTK 可绘制的 bitmap */
    if (gl_view->use_readback) {
        /*
         * 回读模式（方案 A）：glReadPixels 从 OGRE 纹理回读到 CPU 内存
         * 对齐 fbo_to_bitmap.md 方案 A 和 test/triangle_fbo_to_awtk/main.c
         */
        uint32_t old_flags = 0;
        rect_t src_rect;
        rect_t dst_rect;
        uint8_t* data = bitmap_lock_buffer_for_write(gl_view->bitmap);
        if (data) {
            ogre_awtk_readback(ext->ogre_app_ptr, data, widget->w, widget->h);
            bitmap_unlock_buffer(gl_view->bitmap);
            bitmap_set_dirty(gl_view->bitmap, TRUE);
        }

        /* 清除 GPU 纹理标志，强制 AWTK 使用 CPU 路径采样 */
        old_flags = gl_view->bitmap->flags;
        gl_view->bitmap->flags &= ~(BITMAP_FLAG_TEXTURE | BITMAP_FLAG_GPU_FBO_TEXTURE);

        src_rect = rect_init(0, 0, gl_view->bitmap->w, gl_view->bitmap->h);
        dst_rect = rect_init(0, 0, widget->w, widget->h);
        canvas_draw_image(c, gl_view->bitmap, &src_rect, &dst_rect);

        gl_view->bitmap->flags = old_flags;

    } else if (gl_view->fbo.handle != NULL) {
        /*
         * 零拷贝模式（方案 B）：OGRE 纹理 → blit 到 AWTK FBO → GPU 直接采样
         * OGRE renderOneFrame 内部绑定自身 FBO，结果在 offscreen_tex_id 中。
         * 通过 blit（全屏四边形 + 纹理采样）将 OGRE 纹理复制到 AWTK 的 offline_fbo。
         */
        rect_t src_rect;
        rect_t dst_rect;

        ogre_awtk_blit_to_fbo(ext->ogre_app_ptr,
                              gl_view->fbo.offline_fbo,
                              widget->w, widget->h);

        /* bitmap_set_dirty(FALSE) 防止 AWTK 从 CPU 缓冲区重新加载纹理 */
        bitmap_set_dirty(gl_view->bitmap, FALSE);

        src_rect = rect_init(0, 0, gl_view->bitmap->w, gl_view->bitmap->h);
        dst_rect = rect_init(0, 0, widget->w, widget->h);
        canvas_draw_image(c, gl_view->bitmap, &src_rect, &dst_rect);
    }

    return RET_OK;
}

/* ------------------------------------------------------------------ */
/*  Destroy                                                            */
/* ------------------------------------------------------------------ */

static ret_t yps_gl_view_on_destroy(widget_t* widget) {
    yps_gl_view_t* gl_view = YPS_GL_VIEW(widget);
    yps_gl_view_ext_t* ext = NULL;
    vgcanvas_t* vg = NULL;
    widget_t* wm = NULL;
    return_value_if_fail(gl_view != NULL, RET_BAD_PARAMS);

    ext = YPS_GL_VIEW_EXT(gl_view);

    /* 1. 先销毁 OGRE（释放 GL 上下文和资源） */
    if (ext->ogre_app_ptr != NULL) {
        ogre_awtk_deinit(ext->ogre_app_ptr);
        ext->ogre_app_ptr = NULL;
    }

    /* 2. 销毁 AWTK FBO 和 bitmap（需要 vgcanvas，通过 window_manager 获取） */
    wm = window_manager();
    if (wm != NULL) {
        canvas_t* canvas = widget_get_canvas(wm);
        if (canvas != NULL) {
            vg = canvas_get_vgcanvas(canvas);
        }
    }
    yps_gl_view_release_surface(gl_view, vg);

    /* 3. 释放字符串与模型列表 */
    TKMEM_FREE(gl_view->scene_file);
    TKMEM_FREE(gl_view->content_dir);
    yps_gl_view_clear_models(gl_view);

    return RET_OK;
}

/* ------------------------------------------------------------------ */
/*  Properties                                                         */
/* ------------------------------------------------------------------ */

static ret_t yps_gl_view_get_prop(widget_t* widget, const char* name, value_t* v) {
    yps_gl_view_t* gl_view = YPS_GL_VIEW(widget);
    return_value_if_fail(gl_view != NULL && name != NULL && v != NULL, RET_BAD_PARAMS);

    if (tk_str_eq(name, YPS_GL_VIEW_PROP_SCENE_FILE)) {
        value_set_str(v, gl_view->scene_file);
        return RET_OK;
    } else if (tk_str_eq(name, YPS_GL_VIEW_PROP_CONTENT_DIR)) {
        value_set_str(v, gl_view->content_dir);
        return RET_OK;
    } else if (tk_str_eq(name, YPS_GL_VIEW_PROP_USE_READBACK)) {
        value_set_bool(v, gl_view->use_readback);
        return RET_OK;
    } else if (tk_str_eq(name, YPS_GL_VIEW_PROP_TARGET_FPS)) {
        value_set_uint32(v, YPS_GL_VIEW_EXT(gl_view)->target_fps);
        return RET_OK;
    } else if (tk_str_eq(name, YPS_GL_VIEW_PROP_MODEL_LIST)) {
        str_t str;
        int32_t i = 0;

        str_init(&str, 64);
        for (i = 0; i < gl_view->mode_count; i++) {
            if (i > 0) str_append_char(&str, ';');
            str_append(&str, gl_view->mode_lists[i]);
        }
        value_dup_str(v, str.str);
        str_reset(&str);

        return RET_OK;
    }

    return RET_NOT_FOUND;
}

static ret_t yps_gl_view_set_prop(widget_t* widget, const char* name, const value_t* v) {
    return_value_if_fail(name != NULL && v != NULL, RET_BAD_PARAMS);

    if (tk_str_eq(name, YPS_GL_VIEW_PROP_SCENE_FILE)) {
        return yps_gl_view_set_scene_file(widget, value_str(v));
    } else if (tk_str_eq(name, YPS_GL_VIEW_PROP_CONTENT_DIR)) {
        return yps_gl_view_set_content_dir(widget, value_str(v));
    } else if (tk_str_eq(name, YPS_GL_VIEW_PROP_MODEL_LIST)) {
        return yps_gl_view_set_model_list(widget, value_str(v));
    } else if (tk_str_eq(name, YPS_GL_VIEW_PROP_USE_READBACK)) {
        return yps_gl_view_set_readback(widget, value_bool(v));
    } else if (tk_str_eq(name, YPS_GL_VIEW_PROP_TARGET_FPS)) {
        return yps_gl_view_set_target_fps(widget, value_uint32(v));
    }

    return RET_NOT_FOUND;
}

/* ------------------------------------------------------------------ */
/*  VTable & Create                                                    */
/* ------------------------------------------------------------------ */

static const char* s_yps_gl_view_properties[] = {
    YPS_GL_VIEW_PROP_SCENE_FILE,
    YPS_GL_VIEW_PROP_MODEL_LIST,
    YPS_GL_VIEW_PROP_CONTENT_DIR,
    YPS_GL_VIEW_PROP_USE_READBACK,
    YPS_GL_VIEW_PROP_TARGET_FPS,
    NULL
};

TK_DECL_VTABLE(yps_gl_view) = {
    .size = sizeof(yps_gl_view_t) + sizeof(yps_gl_view_ext_t),
    .type = WIDGET_TYPE_YPS_GL_VIEW,
    .clone_properties = s_yps_gl_view_properties,
    .persistent_properties = s_yps_gl_view_properties,
    .get_parent_vt = TK_GET_PARENT_VTABLE(widget),
    .create = yps_gl_view_create,
    .on_paint_self = yps_gl_view_on_paint_self,
    .on_destroy = yps_gl_view_on_destroy,
    .get_prop = yps_gl_view_get_prop,
    .set_prop = yps_gl_view_set_prop,
};

widget_t* yps_gl_view_create(widget_t* parent, xy_t x, xy_t y, wh_t w, wh_t h) {
    widget_t* widget = widget_create(parent, TK_REF_VTABLE(yps_gl_view), x, y, w, h);
    yps_gl_view_t* gl_view = YPS_GL_VIEW(widget);
    yps_gl_view_ext_t* ext = NULL;
    return_value_if_fail(gl_view != NULL, NULL);

    gl_view->use_readback = FALSE;
    gl_view->bitmap = NULL;
    gl_view->scene_file = NULL;
    gl_view->mode_lists = NULL;
    gl_view->mode_count = 0;
    gl_view->content_dir = NULL;
    memset(&(gl_view->fbo), 0, sizeof(gl_view->fbo));

    ext = YPS_GL_VIEW_EXT(gl_view);
    memset(ext, 0, sizeof(yps_gl_view_ext_t));

    /* 添加定时器驱动持续渲染（~30FPS），对齐 test/triangle_fbo_to_awtk/main.c */
    ext->target_fps = YPS_GL_VIEW_DEFAULT_FPS;
    ext->timer_id = widget_add_timer(widget, on_invalidate_timer, 1000 / ext->target_fps);

    return widget;
}

widget_t* yps_gl_view_cast(widget_t* widget) {
    return_value_if_fail(WIDGET_IS_INSTANCE_OF(widget, yps_gl_view), NULL);

    return widget;
}

/* ------------------------------------------------------------------ */
/*  Public API                                                         */
/* ------------------------------------------------------------------ */

ret_t yps_gl_view_set_scene_file(widget_t* widget, const char* scene_file) {
    yps_gl_view_t* gl_view = YPS_GL_VIEW(widget);
    char* tmp = NULL;
    return_value_if_fail(gl_view != NULL, RET_BAD_PARAMS);

    /* 先复制再释放：scene_file 可能就是 gl_view->scene_file 本身 */
    tmp = (scene_file != NULL) ? tk_strdup(scene_file) : NULL;
    return_value_if_fail(scene_file == NULL || tmp != NULL, RET_OOM);

    TKMEM_FREE(gl_view->scene_file);
    gl_view->scene_file = tmp;

    YPS_GL_VIEW_EXT(gl_view)->scene_dirty = TRUE;
    widget_invalidate_force(widget, NULL);

    return RET_OK;
}

const char* yps_gl_view_get_scene_file(widget_t* widget) {
    yps_gl_view_t* gl_view = YPS_GL_VIEW(widget);
    return_value_if_fail(gl_view != NULL, NULL);

    return gl_view->scene_file;
}

ret_t yps_gl_view_set_models(widget_t* widget, const char** files, uint32_t count) {
    yps_gl_view_t* gl_view = YPS_GL_VIEW(widget);
    return_value_if_fail(gl_view != NULL, RET_BAD_PARAMS);

    yps_gl_view_clear_models(gl_view);

    if (files != NULL && count > 0) {
        uint32_t i = 0;
        uint32_t n = 0;
        char** arr = TKMEM_ZALLOCN(char*, count);
        return_value_if_fail(arr != NULL, RET_OOM);

        for (i = 0; i < count; i++) {
            if (files[i] == NULL || files[i][0] == '\0') continue;
            arr[n] = tk_strdup(files[i]);
            if (arr[n] != NULL) n++;
        }

        if (n > 0) {
            gl_view->mode_lists = arr;
            gl_view->mode_count = (int32_t)n;
        } else {
            TKMEM_FREE(arr);
        }
    }

    YPS_GL_VIEW_EXT(gl_view)->scene_dirty = TRUE;
    widget_invalidate_force(widget, NULL);

    return RET_OK;
}

ret_t yps_gl_view_set_model_list(widget_t* widget, const char* model_list) {
    yps_gl_view_t* gl_view = YPS_GL_VIEW(widget);
    char* dup = NULL;
    char** tokens = NULL;
    uint32_t max = 1;
    uint32_t n = 0;
    const char* p = NULL;
    ret_t ret = RET_OK;
    return_value_if_fail(gl_view != NULL, RET_BAD_PARAMS);

    if (model_list == NULL || model_list[0] == '\0') {
        return yps_gl_view_set_models(widget, NULL, 0);
    }

    for (p = model_list; *p != '\0'; p++) {
        if (*p == ';' || *p == ',') max++;
    }

    dup = tk_strdup(model_list);
    return_value_if_fail(dup != NULL, RET_OOM);

    tokens = TKMEM_ZALLOCN(char*, max);
    if (tokens == NULL) {
        TKMEM_FREE(dup);
        return RET_OOM;
    }

    n = yps_gl_view_split_models(dup, tokens, max);
    ret = yps_gl_view_set_models(widget, (const char**)tokens, tk_min(n, max));

    TKMEM_FREE(tokens);
    TKMEM_FREE(dup);

    return ret;
}

ret_t yps_gl_view_set_content_dir(widget_t* widget, const char* content_dir) {
    yps_gl_view_t* gl_view = YPS_GL_VIEW(widget);
    char* tmp = NULL;
    return_value_if_fail(gl_view != NULL, RET_BAD_PARAMS);

    tmp = (content_dir != NULL) ? tk_strdup(content_dir) : NULL;
    return_value_if_fail(content_dir == NULL || tmp != NULL, RET_OOM);

    TKMEM_FREE(gl_view->content_dir);
    gl_view->content_dir = tmp;

    /* 资源目录变了，允许重新尝试初始化 */
    YPS_GL_VIEW_EXT(gl_view)->init_failed = FALSE;

    return RET_OK;
}

const char* yps_gl_view_get_content_dir(widget_t* widget) {
    yps_gl_view_t* gl_view = YPS_GL_VIEW(widget);
    return_value_if_fail(gl_view != NULL, NULL);

    return gl_view->content_dir;
}

ret_t yps_gl_view_set_readback(widget_t* widget, bool_t use_readback) {
    yps_gl_view_t* gl_view = YPS_GL_VIEW(widget);
    return_value_if_fail(gl_view != NULL, RET_BAD_PARAMS);

    if (gl_view->use_readback != use_readback) {
        gl_view->use_readback = use_readback;
        /* 两种模式的离屏资源不同，下一帧重建 */
        YPS_GL_VIEW_EXT(gl_view)->surface_dirty = TRUE;
        widget_invalidate_force(widget, NULL);
    }

    return RET_OK;
}

ret_t yps_gl_view_switch_scene(widget_t* widget, const char* scene_file) {
    return yps_gl_view_set_scene_file(widget, scene_file);
}

ret_t yps_gl_view_reload_scene(widget_t* widget) {
    yps_gl_view_t* gl_view = YPS_GL_VIEW(widget);
    return_value_if_fail(gl_view != NULL, RET_BAD_PARAMS);

    YPS_GL_VIEW_EXT(gl_view)->scene_dirty = TRUE;
    widget_invalidate_force(widget, NULL);

    return RET_OK;
}

ret_t yps_gl_view_unload_scene(widget_t* widget) {
    yps_gl_view_t* gl_view = YPS_GL_VIEW(widget);
    yps_gl_view_ext_t* ext = NULL;
    return_value_if_fail(gl_view != NULL, RET_BAD_PARAMS);

    ext = YPS_GL_VIEW_EXT(gl_view);
    if (ext->ogre_app_ptr != NULL) {
        ogre_awtk_clear_scene(ext->ogre_app_ptr);
    }

    /* 保留 scene_file / 模型列表，之后可用 reload_scene 恢复 */
    ext->scene_dirty = FALSE;
    widget_invalidate_force(widget, NULL);

    return RET_OK;
}

ret_t yps_gl_view_set_target_fps(widget_t* widget, uint32_t fps) {
    yps_gl_view_t* gl_view = YPS_GL_VIEW(widget);
    yps_gl_view_ext_t* ext = NULL;
    return_value_if_fail(gl_view != NULL && fps > 0, RET_BAD_PARAMS);

    ext = YPS_GL_VIEW_EXT(gl_view);
    if (ext->timer_id != TK_INVALID_ID) {
        timer_remove(ext->timer_id);
        ext->timer_id = TK_INVALID_ID;
    }

    ext->target_fps = fps;
    ext->timer_id = widget_add_timer(widget, on_invalidate_timer, 1000 / fps);

    return RET_OK;
}

uint32_t yps_gl_view_get_target_fps(widget_t* widget) {
    yps_gl_view_t* gl_view = YPS_GL_VIEW(widget);
    return_value_if_fail(gl_view != NULL, 0);

    return YPS_GL_VIEW_EXT(gl_view)->target_fps;
}
