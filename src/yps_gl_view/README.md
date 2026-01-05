# YPS GL View控件

YPS GL View是一个3D OpenGL视图控件，用于在AWTK应用程序中渲染3D场景。该控件基于Horde3D引擎实现。

## 功能特性

- 渲染3D场景
- 支持自定义渲染管线
- 与AWTK UI系统无缝集成
- 响应控件大小变化
- 直接在on_paint回调中渲染，无需定时器

## 属性

| 属性 | 类型 | 描述 |
|------|------|------|
| scene_file | string | 3D场景文件路径 |
| pipeline_file | string | 渲染管线文件路径 |

## 使用方法

### XML定义

```xml
<yps_gl_view x="0" y="0" w="100%" h="100%" scene_file="model.scene.xml" 
             pipeline_file="pipeline.xml"/>
```

### C代码

```c
widget_t* yps_gl_view = yps_gl_view_create(parent, 0, 0, 300, 300);
yps_gl_view_set_scene_file(yps_gl_view, "model.scene.xml");
yps_gl_view_set_pipeline_file(yps_gl_view, "pipeline.xml");
```

## 构建要求

- AWTK库
- Horde3D引擎
- OpenGL支持

## 注意事项

- 场景文件和渲染管线文件需要预先准备
- 需要确保系统支持OpenGL
- 控件会在每次绘制时渲染3D场景