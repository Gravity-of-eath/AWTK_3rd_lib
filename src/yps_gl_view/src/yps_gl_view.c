/**
 * File:   yps_gl_view.c
 * Author: AWTK Develop Team
 * Brief:  3D OpenGL视图控件，使用Horde3D渲染3D场景
 *
 * Copyright (c) 2018-2025 Guangzhou ZHIYUAN Electronics Co.,Ltd.
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
 * 2025-01-10 Li XianJing <xianjimli@hotmail.com> created
 *
 */

#include "base/bitmap.h"
#include "base/canvas.h"
#include "base/enums.h"
#include "base/image_loader.h"
#include "base/image_manager.h"
#include "base/keys.h"
#include "base/native_window.h"
#include "base/system_info.h"
#include "base/widget_consts.h"
#include "base/window.h"
#include "base/window_manager.h"
#include "tkc/mem.h"
#include "tkc/str.h"
#include "tkc/time_now.h"
#include "yps_gl_view.h"
#include "horde3d/horde3d.h"
#include "horde3d/egInit.h"
#include "horde3d/Horde3DUtils.h"
#include "horde3d/Horde3DOverlays.h"

static void initScenes(yps_gl_view_t *yps_gl_view)
{
  yps_gl_view->window_data = createWindow((char *)"YPS_GL_View",
                                          yps_gl_view->widget.w, yps_gl_view->widget.h, true, 45.0f, 0.1f, 1000.0f);
  // 检查是否有有效的窗口数据
  if (yps_gl_view->window_data != NULL)
  {
    // 初始化Horde3D
    if (h3dGetNextResource(0, H3DResTypes::Undefined) == 0)
    {
      if (!h3dInit())
      {
        log_error("Failed to initialize Horde3D\n");
        return RET_FAIL;
      }

      // Set options
      h3dSetOption(H3DOptions::LoadTextures, 1);
      h3dSetOption(H3DOptions::TexCompression, 0);
      h3dSetOption(H3DOptions::MaxAnisotropy, 4);
      h3dSetOption(H3DOptions::ShadowMapSize, 2048);
      h3dSetOption(H3DOptions::FastAnimation, 1);
      h3dSetOption(H3DOptions::SampleCount, 0);
      h3dSetOption(H3DOptions::DumpFailedShaders, 1);
      h3dSetOption(H3DOptions::GatherTimeStats, 1);
    }

    // Load scene file if specified and not already loaded
    if (yps_gl_view->scene_file != NULL && h3dGetNodeParamI(H3DNode(0), H3DNodeParams::NodeTypeI) == 0)
    {

      // 1. Add resources
      h3dSetOption(H3DOptions::FastAnimation, 0);

      // Environment
      H3DRes envRes = h3dAddResource(H3DResTypes::SceneGraph,
                                     "models/sphere/sphere.scene.xml", 0);

      // Knight
      H3DRes knightRes = 0;
        knightRes = h3dAddResource(H3DResTypes::SceneGraph,
                                   "models/knight/knightES.scene.xml", 0);

      H3DRes knightAnim1Res =
          h3dAddResource(H3DResTypes::Animation, "animations/knight_order.anim", 0);
      H3DRes knightAnim2Res = h3dAddResource(H3DResTypes::Animation,
                                             "animations/knight_attack.anim", 0);

      // Shader for deferred shading
      H3DRes lightMatRes =
          h3dAddResource(H3DResTypes::Material, "materials/light.material.xml", 0);

      // Particle system
      H3DRes particleSysRes =
          h3dAddResource(H3DResTypes::SceneGraph,
                         "particles/particleSys1/particleSys1.scene.xml", 0);

      // Help info
      _helpLabels[_helpRows - 1] = "1/2:";
      _helpValues[_helpRows - 1] = "Animation blending";

      // 2. Load resources

      if (h3dutLoadResourcesFromDisk("/data/h3d/Content"))
      {
        h3dutDumpMessages();
        return false;
      }

      // 3. Add scene nodes

      // Add camera
      yps_gl_view->_cam = h3dAddCameraNode(H3DRootNode, "Camera", getPipelineRes());
      // h3dSetNodeParamI( _cam, H3DCamera::OccCullingI, 1 );

      // Add environment
      H3DNode env = h3dAddNodes(H3DRootNode, envRes);
      h3dSetNodeTransform(env, 0, -20, 0, 0, 0, 0, 20, 20, 20);

      // Add knight
      _knight = h3dAddNodes(H3DRootNode, knightRes);
      h3dSetNodeTransform(_knight, 0, 0, 0, 0, 180, 0, 0.1f, 0.1f, 0.1f);
      h3dSetupModelAnimStage(_knight, 0, knightAnim1Res, 0, "", false);
      h3dSetupModelAnimStage(_knight, 1, knightAnim2Res, 0, "", false);

      // Attach particle system to hand joint
      h3dFindNodes(_knight, "Bip01_R_Hand", H3DNodeTypes::Joint);
      H3DNode hand = h3dGetNodeFindResult(0);
      _particleSys = h3dAddNodes(hand, particleSysRes);
      h3dSetNodeTransform(_particleSys, 0, 40, 0, 90, 0, 0, 1, 1, 1);

      // Add light source
      H3DNode light = h3dAddLightNode(H3DRootNode, "Light1", lightMatRes,
                                      "LIGHTING", "SHADOWMAP");
      h3dSetNodeTransform(light, 0, 15, 10, -60, 0, 0, 1, 1, 1);
      h3dSetNodeParamF(light, H3DLight::RadiusF, 0, 30);
      h3dSetNodeParamF(light, H3DLight::FovF, 0, 90);
      h3dSetNodeParamI(light, H3DLight::ShadowMapCountI, 1);
      h3dSetNodeParamF(light, H3DLight::ShadowMapBiasF, 0, 0.01f);
      h3dSetNodeParamF(light, H3DLight::ColorF3, 0, 1.0f);
      h3dSetNodeParamF(light, H3DLight::ColorF3, 1, 0.8f);
      h3dSetNodeParamF(light, H3DLight::ColorF3, 2, 0.7f);
      h3dSetNodeParamF(light, H3DLight::ColorMultiplierF, 0, 1.0f);

      // Customize post processing effects
      H3DNode matRes =
          h3dFindResource(H3DResTypes::Material, "pipelines/postHDR.material.xml");
      h3dSetMaterialUniform(matRes, "hdrExposure", 2.5f, 0, 0, 0);
      h3dSetMaterialUniform(matRes, "hdrBrightThres", 0.5f, 0, 0, 0);
      h3dSetMaterialUniform(matRes, "hdrBrightOffset", 0.08f, 0, 0, 0);
    }
  }

  static ret_t yps_gl_view_on_destroy(widget_t * widget)
  {
    yps_gl_view_t *yps_gl_view = YPS_GL_VIEW(widget);
    return_value_if_fail(yps_gl_view != NULL, RET_BAD_PARAMS);

    // Clean up resources
    if (yps_gl_view->window_data != NULL)
    {
      destroyWindow(yps_gl_view->window_data);
      yps_gl_view->window_data = NULL;
    }

    // Shutdown Horde3D
    h3dShutdown();

    // Free string properties
    TKMEM_FREE(yps_gl_view->scene_file);
    TKMEM_FREE(yps_gl_view->pipeline_file);

    return RET_OK;
  }

  static ret_t yps_gl_view_on_paint(widget_t * widget, canvas_t * c)
  {
    yps_gl_view_t *yps_gl_view = YPS_GL_VIEW(widget);
    return_value_if_fail(yps_gl_view != NULL && c != NULL, RET_BAD_PARAMS);

    // Show stats
    // h3dShowFrameStats(0, 0, 0); // 简单的统计信息

    // 渲染场景
    if (h3dRender(yps_gl_view->_cam))
    {
      h3dFinalizeFrame();

      // 获取帧缓冲区图像
      FbData *fbData = getFbImage();
      if (fbData != NULL && fbData->data != NULL)
      {
        // 创建AWTK bitmap对象
        bitmap_t bitmap;
        ret_t ret = bitmap_init(&bitmap, fbData->width, fbData->height,
                                BITMAP_FMT_RGBA8888, fbData->data);

        if (ret == RET_OK)
        {
          // 设置源和目标矩形
          rect_t src = {0, 0, fbData->width, fbData->height};
          rect_t dst = {0, 0, widget->w, widget->h};

          // 绘制图像到画布
          canvas_draw_image_scale_cut(c, &bitmap, &src, &dst);

          // 释放bitmap数据
          bitmap_destroy(&bitmap);
        }

        // 释放帧缓冲区数据
        release();
      }
    }
  }

  return RET_OK;
}

static ret_t yps_gl_view_on_event(widget_t *widget, event_t *e)
{
  yps_gl_view_t *yps_gl_view = YPS_GL_VIEW(widget);
  return_value_if_fail(widget != NULL && yps_gl_view != NULL, RET_BAD_PARAMS);

  switch (e->type)
  {
  case EVT_WIDGET_LOAD:
  {
    // 控件加载完成后初始化3D环境
    if (yps_gl_view->window_data == NULL)
    {
      // Create window data using createWindow function
      yps_gl_view->window_data = createWindow((char *)"YPS_GL_View",
                                              widget->w, widget->h, true, 45.0f, 0.1f, 1000.0f);

      if (yps_gl_view->window_data != NULL)
      {
        // Initialize Horde3D
        if (!h3dInit())
        {
          log_error("Failed to initialize Horde3D\n");
          destroyWindow(yps_gl_view->window_data);
          yps_gl_view->window_data = NULL;
          return RET_FAIL;
        }

        // Set options
        h3dSetOption(H3DOptions::LoadTextures, 1);
        h3dSetOption(H3DOptions::TexCompression, 0);
        h3dSetOption(H3DOptions::MaxAnisotropy, 4);
        h3dSetOption(H3DOptions::ShadowMapSize, 2048);
        h3dSetOption(H3DOptions::FastAnimation, 1);
        h3dSetOption(H3DOptions::SampleCount, 0);
        h3dSetOption(H3DOptions::DumpFailedShaders, 1);
        h3dSetOption(H3DOptions::GatherTimeStats, 1);

        // Load scene file if specified
        if (yps_gl_view->scene_file != NULL)
        {
          H3DRes scene = h3dLoadScene(yps_gl_view->scene_file, 0);
          if (scene == 0)
          {
            log_warn("Failed to load scene file: %s\n", yps_gl_view->scene_file);
          }
        }

        // Load pipeline file if specified
        if (yps_gl_view->pipeline_file != NULL)
        {
          H3DRes pipeline = h3dLoadPipeline(yps_gl_view->pipeline_file);
          if (pipeline == 0)
          {
            log_warn("Failed to load pipeline file: %s\n", yps_gl_view->pipeline_file);
          }
        }
      }
    }
    break;
  }
  case EVT_RESIZE:
  case EVT_MOVE_RESIZE:
  {
    /* 控件大小改变时，调整3D渲染窗口大小 */
    if (widget->w > 0 && widget->h > 0)
    {
      // 更新窗口数据的尺寸
      if (yps_gl_view->window_data != NULL)
      {
        yps_gl_view->window_data->width = widget->w;
        yps_gl_view->window_data->height = widget->h;
      }

      // 重新调整渲染管线大小
      H3DNode cam = h3dGetNodeByName(0, "Camera");
      if (cam)
      {
        h3dResizePipelineBuffers(h3dGetNodeParamI(cam, H3DCamera::PipeResI), widget->w, widget->h);
      }
    }
    break;
  }
  default:
    break;
  }

  return RET_OK;
}

ret_t yps_gl_view_set_scene_file(widget_t *widget, const char *scene_file)
{
  yps_gl_view_t *yps_gl_view = YPS_GL_VIEW(widget);
  return_value_if_fail(yps_gl_view != NULL, RET_BAD_PARAMS);

  return str_set(&(yps_gl_view->scene_file), scene_file);
}

ret_t yps_gl_view_set_pipeline_file(widget_t *widget,
                                    const char *pipeline_file)
{
  yps_gl_view_t *yps_gl_view = YPS_GL_VIEW(widget);
  return_value_if_fail(yps_gl_view != NULL, RET_BAD_PARAMS);

  return str_set(&(yps_gl_view->pipeline_file), pipeline_file);
}

static ret_t yps_gl_view_get_prop(widget_t *widget, const char *name,
                                  value_t *v)
{
  yps_gl_view_t *yps_gl_view = YPS_GL_VIEW(widget);
  return_value_if_fail(yps_gl_view != NULL && name != NULL && v != NULL,
                       RET_BAD_PARAMS);

  if (tk_str_eq(name, YPS_GL_VIEW_PROP_SCENE_FILE))
  {
    value_set_str(v, yps_gl_view->scene_file);
    return RET_OK;
  }
  else if (tk_str_eq(name, YPS_GL_VIEW_PROP_PIPELINE_FILE))
  {
    value_set_str(v, yps_gl_view->pipeline_file);
    return RET_OK;
  }

  return RET_NOT_FOUND;
}

static ret_t yps_gl_view_set_prop(widget_t *widget, const char *name,
                                  const value_t *v)
{
  return_value_if_fail(widget != NULL && name != NULL && v != NULL,
                       RET_BAD_PARAMS);

  if (tk_str_eq(name, YPS_GL_VIEW_PROP_SCENE_FILE))
  {
    yps_gl_view_set_scene_file(widget, value_str(v));
    return RET_OK;
  }
  else if (tk_str_eq(name, YPS_GL_VIEW_PROP_PIPELINE_FILE))
  {
    yps_gl_view_set_pipeline_file(widget, value_str(v));
    return RET_OK;
  }

  return RET_NOT_FOUND;
}

TK_DECL_VTABLE(yps_gl_view) = {
    .size = sizeof(yps_gl_view_t),
    .type = WIDGET_TYPE_YPS_GL_VIEW,
    .parent = TK_PARENT_VTABLE(widget),
    .create = yps_gl_view_create,
    .on_paint_children = yps_gl_view_on_paint,
    .on_destroy = yps_gl_view_on_destroy,
    .on_event = yps_gl_view_on_event,
    .set_prop = yps_gl_view_set_prop,
    .get_prop = yps_gl_view_get_prop,
};

widget_t *yps_gl_view_create(widget_t *parent, xy_t x, xy_t y, wh_t w, wh_t h)
{
  widget_t *widget =
      widget_create(parent, TK_REF_VTABLE(yps_gl_view), x, y, w, h);
  yps_gl_view_t *yps_gl_view = YPS_GL_VIEW(widget);
  return_value_if_fail(yps_gl_view != NULL, NULL);

  yps_gl_view->scene_file = NULL;
  yps_gl_view->pipeline_file = NULL;
  yps_gl_view->window_data = NULL;

  return widget;
}