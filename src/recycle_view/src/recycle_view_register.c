#include "base/widget_factory.h"
#include "recycle_view.h"
#include "recycle_view_register.h"

ret_t recycle_view_register(void) {
  return widget_factory_register(widget_factory(), WIDGET_TYPE_RECYCLE_VIEW, recycle_view_create);
}

const char* recycle_view_supported_render_mode(void) {
  return "OpenGL|AGGE-BGR565|AGGE-BGRA8888|AGGE-MONO";
}
