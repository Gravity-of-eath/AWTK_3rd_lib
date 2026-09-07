#ifndef OGRE_AWTK_APP_HPP
#define OGRE_AWTK_APP_HPP

#ifdef __cplusplus
extern "C" {
#endif

/**
 * 初始化 OGRE 离屏渲染上下文（只建立上下文与相机/灯光，不加载任何场景内容）。
 * 场景内容请在初始化后调用 ogre_awtk_load_scene / ogre_awtk_load_models 加载。
 * @param content_dir 资源目录路径（可为 NULL），同时用于查找 plugins.cfg / resources.cfg
 * @param w 离屏宽度
 * @param h 离屏高度
 * @return 上下文指针，失败返回 NULL
 */
void* ogre_awtk_init(const char* content_dir, int w, int h);

/**
 * 调整离屏渲染目标尺寸，成功后离屏纹理 ID 会被重新获取。
 * @return 0 成功，负值失败
 */
int ogre_awtk_resize(void* app_ptr, int w, int h);

/**
 * 加载 .scene 场景文件（依赖 Plugin_DotScene）。
 * 会先清空当前场景内容，加载完成后自动把相机对准场景包围盒。
 * scene_file 为 NULL 或空串时等价于 ogre_awtk_clear_scene。
 * @return 0 成功，负值失败
 */
int ogre_awtk_load_scene(void* app_ptr, const char* scene_file);

/**
 * 加载模型（.mesh）列表，每个模型挂到独立的子节点上。
 * 会先清空当前场景内容，加载完成后自动把相机对准所有模型的包围盒。
 * @param files 模型文件路径数组
 * @param count 模型个数
 * @return 0 全部成功；>0 表示成功加载的个数少于 count；负值失败
 */
int ogre_awtk_load_models(void* app_ptr, const char* const* files, int count);

/**
 * 清空场景内容（保留相机、主灯光与 GL 上下文）。
 * @return 0 成功，负值失败
 */
int ogre_awtk_clear_scene(void* app_ptr);

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
