#include "ogre_awtk_app.hpp"

#include <string>
#include <vector>

#include "Ogre.h"
#include "OgreShaderGenerator.h"

#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <stdio.h>

#define LOG_TAG "[ogre_awtk] "
#define LOGD(fmt, ...) printf(LOG_TAG fmt "\n", ##__VA_ARGS__)
#define LOGE(fmt, ...) fprintf(stderr, LOG_TAG "ERROR: " fmt "\n", ##__VA_ARGS__)

/* ------------------------------------------------------------------ */
/*  Blit shader (GLES2 fullscreen quad for texture copy)               */
/* ------------------------------------------------------------------ */

static const char* s_blit_vs =
    "attribute vec2 aPos;\n"
    "varying vec2 vUV;\n"
    "void main() {\n"
    "  vUV = aPos * 0.5 + 0.5;\n"
    "  gl_Position = vec4(aPos, 0.0, 1.0);\n"
    "}\n";

static const char* s_blit_fs =
    "precision mediump float;\n"
    "varying vec2 vUV;\n"
    "uniform sampler2D uTex;\n"
    "void main() {\n"
    "  gl_FragColor = texture2D(uTex, vUV);\n"
    "}\n";

static GLuint compile_gl_shader(GLenum type, const char* src) {
    GLuint sh = glCreateShader(type);
    glShaderSource(sh, 1, &src, NULL);
    glCompileShader(sh);
    GLint ok = 0;
    glGetShaderiv(sh, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetShaderInfoLog(sh, sizeof(log), NULL, log);
        LOGE("shader compile failed: %s", log);
    }
    return sh;
}

/* ------------------------------------------------------------------ */
/*  Context                                                            */
/* ------------------------------------------------------------------ */

struct ogre_awtk_ctx_t {
    Ogre::Root* root = nullptr;
    Ogre::RenderWindow* window = nullptr;
    Ogre::SceneManager* scene_mgr = nullptr;
    Ogre::Camera* camera = nullptr;
    Ogre::SceneNode* cam_node = nullptr;
    Ogre::RTShader::ShaderGenerator* shader_gen = nullptr;
    int width = 0;
    int height = 0;
    int32_t offscreen_tex_id = 0;
    std::string scene_file;
    std::string content_dir;

    /* blit / readback helpers */
    GLuint blit_program = 0;
    GLuint blit_vbo = 0;
    GLuint readback_fbo = 0;
    bool blit_inited = false;
};

static void ensure_blit_resources(ogre_awtk_ctx_t* ctx) {
    if (ctx->blit_inited) return;

    GLuint vs = compile_gl_shader(GL_VERTEX_SHADER, s_blit_vs);
    GLuint fs = compile_gl_shader(GL_FRAGMENT_SHADER, s_blit_fs);
    ctx->blit_program = glCreateProgram();
    glAttachShader(ctx->blit_program, vs);
    glAttachShader(ctx->blit_program, fs);
    glLinkProgram(ctx->blit_program);
    glDeleteShader(vs);
    glDeleteShader(fs);

    GLint linked = 0;
    glGetProgramiv(ctx->blit_program, GL_LINK_STATUS, &linked);
    if (!linked) {
        char log[512];
        glGetProgramInfoLog(ctx->blit_program, sizeof(log), NULL, log);
        LOGE("blit program link failed: %s", log);
    }

    /* fullscreen quad VBO: triangle strip covering -1..1 */
    float quad[] = { -1.0f, -1.0f,  1.0f, -1.0f,  -1.0f, 1.0f,  1.0f, 1.0f };
    glGenBuffers(1, &ctx->blit_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, ctx->blit_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    /* readback FBO (reusable, attach texture each time) */
    glGenFramebuffers(1, &ctx->readback_fbo);

    ctx->blit_inited = true;
    LOGD("blit resources initialized: program=%u vbo=%u readback_fbo=%u",
         (unsigned)ctx->blit_program, (unsigned)ctx->blit_vbo, (unsigned)ctx->readback_fbo);
}

static void destroy_blit_resources(ogre_awtk_ctx_t* ctx) {
    if (!ctx->blit_inited) return;
    if (ctx->blit_vbo) { glDeleteBuffers(1, &ctx->blit_vbo); ctx->blit_vbo = 0; }
    if (ctx->blit_program) { glDeleteProgram(ctx->blit_program); ctx->blit_program = 0; }
    if (ctx->readback_fbo) { glDeleteFramebuffers(1, &ctx->readback_fbo); ctx->readback_fbo = 0; }
    ctx->blit_inited = false;
}

/* ------------------------------------------------------------------ */
/*  Resource loading (对齐 ogre_app.cpp::loadResource)                 */
/* ------------------------------------------------------------------ */

static void load_resources(ogre_awtk_ctx_t* ctx) {
    Ogre::ResourceGroupManager& rgm = Ogre::ResourceGroupManager::getSingleton();

    /* 添加 content_dir 本身为 FileSystem 资源位置 */
    if (!ctx->content_dir.empty()) {
        std::string dir = ctx->content_dir;
        if (Ogre::FileSystemLayer::fileExists(dir)) {
            rgm.addResourceLocation(dir, "FileSystem",
                Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
            LOGD("added resource location: %s", dir.c_str());
        } else {
            LOGE("content_dir does not exist: %s", dir.c_str());
        }
    }

    /* 尝试加载 content_dir/resources.cfg（同 ogre_app.cpp 的配置文件方式） */
    std::string res_cfg;
    if (!ctx->content_dir.empty()) {
        res_cfg = ctx->content_dir + "/resources.cfg";
    }
    if (!res_cfg.empty() && Ogre::FileSystemLayer::fileExists(res_cfg)) {
        try {
            Ogre::ConfigFile cf;
            cf.load(res_cfg);
            for (auto& s : cf.getSettingsBySection()) {
                for (auto& t : s.second) {
                    std::string type = t.first;
                    std::string arch = t.second;
                    Ogre::StringUtil::trim(arch);
                    if (arch.empty() || arch[0] == '.') {
                        arch = ctx->content_dir + "/" + arch;
                    }
                    arch = Ogre::FileSystemLayer::resolveBundlePath(arch);
                    if ((type == "Zip" || type == "FileSystem") &&
                        !Ogre::FileSystemLayer::fileExists(arch)) {
                        Ogre::LogManager::getSingleton().logWarning(
                            "resource location '" + arch + "' does not exist - skipping");
                        continue;
                    }
                    rgm.addResourceLocation(arch, type, s.first);
                }
            }
            LOGD("loaded resources.cfg from: %s", res_cfg.c_str());
        } catch (const std::exception& e) {
            LOGE("failed to load resources.cfg: %s", e.what());
        }
    }

    rgm.initialiseAllResourceGroups();
    LOGD("all resource groups initialized");
}

/* ------------------------------------------------------------------ */
/*  Scene setup                                                        */
/* ------------------------------------------------------------------ */

static void setup_basic_scene(ogre_awtk_ctx_t* ctx) {
    ctx->scene_mgr = ctx->root->createSceneManager();
    ctx->scene_mgr->setAmbientLight(Ogre::ColourValue(1.0f, 1.0f, 1.0f));

    Ogre::Light* light = ctx->scene_mgr->createLight("MainLight");
    Ogre::SceneNode* light_node = ctx->scene_mgr->getRootSceneNode()->createChildSceneNode();
    light_node->setPosition(0, 10, 15);
    light_node->attachObject(light);

    ctx->cam_node = ctx->scene_mgr->getRootSceneNode()->createChildSceneNode();
    ctx->cam_node->setPosition(0, 0, 20);
    ctx->cam_node->lookAt(Ogre::Vector3(0, 0, -1), Ogre::Node::TS_PARENT);

    ctx->camera = ctx->scene_mgr->createCamera("yps_gl_view_camera");
    ctx->camera->setNearClipDistance(5);
    ctx->camera->setAutoAspectRatio(true);
    ctx->cam_node->attachObject(ctx->camera);

    ctx->window->addViewport(ctx->camera);
}

/* ------------------------------------------------------------------ */
/*  Public C API                                                       */
/* ------------------------------------------------------------------ */

extern "C" {

void* ogre_awtk_init(const char* scene_file, const char* content_dir, int w, int h) {
    ogre_awtk_ctx_t* ctx = new ogre_awtk_ctx_t();
    ctx->width = w;
    ctx->height = h;
    if (scene_file) ctx->scene_file = scene_file;
    if (content_dir) ctx->content_dir = content_dir;

    LOGD("init: w=%d h=%d scene=%s content=%s",
         w, h, scene_file ? scene_file : "(null)", content_dir ? content_dir : "(null)");

    try {
        /* 对齐 ogre_app.cpp 的初始化流程 */
        ctx->root = new Ogre::Root("./plugins.cfg", "", "ogre_fb0.log");
        ctx->root->loadPlugin("RenderSystem_GLES2");
        ctx->root->setRenderSystem(ctx->root->getAvailableRenderers()[0]);
        ctx->root->initialise(false);

        Ogre::NameValuePairList params;
        params["offScreen"] = "true";
        params["sharedGLContext"] = "true";
        params["offScreenTarget"] = "fboTexture";
        params["fboExternal"] = "false";

        ctx->window = ctx->root->createRenderWindow("YpsGlViewWindow", w, h, false, &params);

        /* 资源加载（RTSS 之前，确保材质可查找） */
        load_resources(ctx);

        /* RTSS */
        if (Ogre::RTShader::ShaderGenerator::initialize()) {
            ctx->shader_gen = Ogre::RTShader::ShaderGenerator::getSingletonPtr();
        }

        /* 场景 */
        setup_basic_scene(ctx);

        if (ctx->shader_gen && ctx->scene_mgr) {
            ctx->shader_gen->addSceneManager(ctx->scene_mgr);
        }

        /* 获取离屏纹理 ID（对齐 ogre_app.cpp） */
        ctx->window->getCustomAttribute("OffscreenColorTexId", &ctx->offscreen_tex_id);
        LOGD("offscreen_tex_id = %d", ctx->offscreen_tex_id);

    } catch (const std::exception& e) {
        LOGE("init failed: %s", e.what());
        ogre_awtk_deinit(ctx);
        return nullptr;
    } catch (...) {
        LOGE("init failed: unknown exception");
        ogre_awtk_deinit(ctx);
        return nullptr;
    }

    return ctx;
}

int ogre_awtk_render_frame(void* app_ptr) {
    ogre_awtk_ctx_t* ctx = static_cast<ogre_awtk_ctx_t*>(app_ptr);
    if (!ctx || !ctx->root) return -1;

    /*
     * renderOneFrame 内部会绑定 OGRE 自己的离屏 FBO（offScreenTarget=fboTexture），
     * 渲染完成后内容在 OGRE 的 offscreen_tex_id 纹理中。
     */
    bool ok = ctx->root->renderOneFrame();

    /* glFinish 确保 GPU 完成渲染，后续采样/回读可安全访问 */
    glFinish();

    return ok ? 0 : -1;
}

int ogre_awtk_blit_to_fbo(void* app_ptr, unsigned int dst_fbo, int w, int h) {
    ogre_awtk_ctx_t* ctx = static_cast<ogre_awtk_ctx_t*>(app_ptr);
    if (!ctx || ctx->offscreen_tex_id <= 0) return -1;

    ensure_blit_resources(ctx);

    /* 保存 GL 状态 */
    GLint prev_fbo = 0, prev_vp[4] = {0}, prev_prog = 0;
    GLint prev_active_tex = 0, prev_tex2d = 0;
    GLboolean prev_depth = GL_FALSE, prev_blend = GL_FALSE, prev_cull = GL_FALSE;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prev_fbo);
    glGetIntegerv(GL_VIEWPORT, prev_vp);
    glGetIntegerv(GL_CURRENT_PROGRAM, &prev_prog);
    glGetIntegerv(GL_ACTIVE_TEXTURE, &prev_active_tex);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &prev_tex2d);
    prev_depth = glIsEnabled(GL_DEPTH_TEST);
    prev_blend = glIsEnabled(GL_BLEND);
    prev_cull  = glIsEnabled(GL_CULL_FACE);

    /* 绑定目标 FBO，用全屏四边形将 OGRE 纹理绘制上去 */
    glBindFramebuffer(GL_FRAMEBUFFER, (GLuint)dst_fbo);
    glViewport(0, 0, w, h);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glDisable(GL_CULL_FACE);

    glUseProgram(ctx->blit_program);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, (GLuint)ctx->offscreen_tex_id);
    glUniform1i(glGetUniformLocation(ctx->blit_program, "uTex"), 0);

    glBindBuffer(GL_ARRAY_BUFFER, ctx->blit_vbo);
    GLint aPos = glGetAttribLocation(ctx->blit_program, "aPos");
    glEnableVertexAttribArray(aPos);
    glVertexAttribPointer(aPos, 2, GL_FLOAT, GL_FALSE, 0, (const void*)0);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glDisableVertexAttribArray(aPos);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glFinish();

    /* 恢复 GL 状态 */
    glUseProgram(prev_prog);
    glActiveTexture(prev_active_tex);
    glBindTexture(GL_TEXTURE_2D, prev_tex2d);
    glBindFramebuffer(GL_FRAMEBUFFER, prev_fbo);
    glViewport(prev_vp[0], prev_vp[1], prev_vp[2], prev_vp[3]);
    if (prev_depth) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
    if (prev_blend) glEnable(GL_BLEND); else glDisable(GL_BLEND);
    if (prev_cull)  glEnable(GL_CULL_FACE); else glDisable(GL_CULL_FACE);

    return 0;
}

int ogre_awtk_readback(void* app_ptr, void* buf, int w, int h) {
    ogre_awtk_ctx_t* ctx = static_cast<ogre_awtk_ctx_t*>(app_ptr);
    if (!ctx || ctx->offscreen_tex_id <= 0 || !buf) return -1;

    ensure_blit_resources(ctx);

    GLint prev_fbo = 0;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prev_fbo);

    /* 将 OGRE 的颜色纹理挂载到 readback FBO 并回读像素 */
    glBindFramebuffer(GL_FRAMEBUFFER, ctx->readback_fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_2D, (GLuint)ctx->offscreen_tex_id, 0);

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        LOGE("readback FBO incomplete: 0x%x", status);
        glBindFramebuffer(GL_FRAMEBUFFER, prev_fbo);
        return -2;
    }

    glReadPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, buf);

    glBindFramebuffer(GL_FRAMEBUFFER, prev_fbo);
    return 0;
}

unsigned int ogre_awtk_get_offscreen_tex_id(void* app_ptr) {
    ogre_awtk_ctx_t* ctx = static_cast<ogre_awtk_ctx_t*>(app_ptr);
    if (!ctx) return 0;
    return (unsigned int)ctx->offscreen_tex_id;
}

/* 兼容旧接口：render_frame + blit_to_fbo */
int ogre_awtk_render_to_fbo(void* app_ptr, unsigned int fbo_id, int w, int h) {
    int ret = ogre_awtk_render_frame(app_ptr);
    if (ret != 0) return ret;
    return ogre_awtk_blit_to_fbo(app_ptr, fbo_id, w, h);
}

void ogre_awtk_deinit(void* app_ptr) {
    ogre_awtk_ctx_t* ctx = static_cast<ogre_awtk_ctx_t*>(app_ptr);
    if (!ctx) return;

    LOGD("deinit");

    /* 先释放 blit/readback GL 资源 */
    destroy_blit_resources(ctx);

    /* 再销毁 OGRE（内部释放 EGL/FBO/texture） */
    if (ctx->root) {
        delete ctx->root;
        ctx->root = nullptr;
        ctx->window = nullptr;
        ctx->scene_mgr = nullptr;
        ctx->camera = nullptr;
        ctx->cam_node = nullptr;
        ctx->shader_gen = nullptr;
    }

    delete ctx;
}

} /* extern "C" */
