基于AWTK的UI控件库
包含一个折线图控件
一个带动画的菜单适配器控件
弧形渐变（非GL实现）
自动按比例缩放子控件的容器
高斯模糊控件
基于缓存策略提高帧率的表盘控件
倒影文本控件
呼吸动画椭圆控件（径向渐变+可配置频率/缩放范围）

breath_ellipse_view 集成方式
1. 在应用启动时注册控件：
   breath_ellipse_view_register();
2. 在UI XML中使用：
   <breath_ellipse_view
     x="c" y="c" w="180" h="180"
     center_color="#00B5FF"
     frequency_bpm="12"
     min_scale="0.8"
     max_scale="1.2"
     duration_ms="0"
     frame_interval_ms="16"/>
3. 可通过API动态控制：
   breath_ellipse_view_set_center_color(widget, "#3D7CFF");
   breath_ellipse_view_set_frequency_bpm(widget, 18);
   breath_ellipse_view_set_scale_range(widget, 0.85f, 1.15f);
   breath_ellipse_view_set_duration_ms(widget, 4000);
   breath_ellipse_view_start(widget);
   breath_ellipse_view_pause(widget);
   breath_ellipse_view_resume(widget);
   breath_ellipse_view_stop(widget);
