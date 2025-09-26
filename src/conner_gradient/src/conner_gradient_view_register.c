#include "conner_gradient_view_register.h"
#include "conner_gradient_view.h"
#include "tkc/mem.h"
#include "tkc/utils.h"
#include "base/widget_factory.h"

ret_t conner_gradient_view_register(void){
  return widget_factory_register(widget_factory(), WIDGET_TYPE_CONNER_GRADIENT_VIEW, conner_gradient_view_create);
}