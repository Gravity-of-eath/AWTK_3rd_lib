#include "base/widget_factory.h"
#include "breath_ellipse_view.h"
#include "breath_ellipse_view_register.h"

ret_t breath_ellipse_view_register(void) {
  return widget_factory_register(widget_factory(), WIDGET_TYPE_BREATH_ELLIPSE_VIEW,
                                 breath_ellipse_view_create);
}

const char* breath_ellipse_view_supported_render_mode(void) {
  return "OpenGL|AGGE-BGR565|AGGE-BGRA8888|AGGE-MONO";
}
