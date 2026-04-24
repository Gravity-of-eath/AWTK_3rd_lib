#include "yps_nes_game_view_register.h"
#include "yps_nes_game_view.h"
#include "base/widget_factory.h"

ret_t yps_nes_game_view_register(void) {
  return widget_factory_register(widget_factory(),
                                 WIDGET_TYPE_YPS_NES_GAME_VIEW,
                                 yps_nes_game_view_create);
}
