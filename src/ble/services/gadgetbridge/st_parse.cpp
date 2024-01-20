#include "ble/services/gadgetbridge/st_parse.hpp"

#include <zephyr/logging/log.h>

#include <ctime>

LOG_MODULE_REGISTER(gadgetbridge_st_parse, CONFIG_NRF_TEST_LOG_LEVEL);

void system::gadgetbridge::st_parse(std::string_view sv)
{
  // (1705719701);E.setTimeZone(-5.0);(s=>s&&(s.timezone=-5.0,require('Storage').write('setting.json',s)))(require('Storage').readJSON('setting.json',1))
  // sv = sv.substr(7);
  // LOG_DBG("ST: %.*s", sv.size(), sv.data());
}