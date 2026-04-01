#ifndef OGRE_AWTK_APP_HPP
#define OGRE_AWTK_APP_HPP

#ifdef __cplusplus
extern "C" {
#endif

/**
 * 初始化 OGRE 离屏渲染上下文。
 * @param scene_file 场景配置文件路径（可为 NULL）
 * @param content_dir 资源目录路径（可为 NULL）
 * @param w 离屏宽度
 * @param h 离屏高度
 * @return 上下文指针，失败返回 NULL
 */
void* ogre_awtk_init(const char* scene_file, const char* content_dir, int w, int h);

/**
 * 渲染一帧到 OGRE 自身的离屏 FBO（offScreenTarget=fboTexture）。
 * 渲染完成后内部调用 glFinish 确保 GPU 完成。
 * @return 0 成功，负值失败
 */
int ogre_awtk_render_frame(void* app_ptr);

/**
 * 将 OGRE 离屏纹理 blit 到目标 FBO（零拷贝路径，GLES2 兼容）。
 * 内部使用全屏四边形 + 纹理采样实现。
 * @param dst_fbo 目标 FBO（通常为 AWTK 的 offline_fbo）
 * @return 0 成功，负值失败
 */
int ogre_awtk_blit_to_fbo(void* app_ptr, unsigned int dst_fbo, int w, int h);

/**
 * 将 OGRE 离屏纹理通过 glReadPixels 回读到 CPU 缓冲区。
 * @param buf RGBA8888 缓冲区，大小至少 w*h*4
 * @return 0 成功，负值失败
 */
int ogre_awtk_readback(void* app_ptr, void* buf, int w, int h);

/**
 * 获取 OGRE 离屏颜色纹理 ID。
 */
unsigned int ogre_awtk_get_offscreen_tex_id(void* app_ptr);

/**
 * 销毁 OGRE 上下文及所有关联资源。
 */
void ogre_awtk_deinit(void* app_ptr);

/**
 * 兼容旧接口：render_frame + blit_to_fbo。
 */
int ogre_awtk_render_to_fbo(void* app_ptr, unsigned int fbo_id, int w, int h);

#ifdef __cplusplus
}
#endif

#endif /* OGRE_AWTK_APP_HPP */
