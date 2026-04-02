#include "base/widget_factory.h"
#include "jump_label.h"
#include "jump_label_register.h"

ret_t jump_label_register(void) {
  return widget_factory_register(widget_factory(), WIDGET_TYPE_JUMP_LABEL, jump_label_create);
}

const char* jump_label_supported_render_mode(void) {
  return "OpenGL|AGGE-BGR565|AGGE-BGRA8888|AGGE-MONO";
}
