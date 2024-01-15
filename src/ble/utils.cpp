#include "ble/utils.hpp"

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(bt_utils, CONFIG_NRF_TEST_LOG_LEVEL);

static const char *day_of_week[] = {
    "Unknown",
    "Monday",
    "Tuesday",
    "Wednesday",
    "Thursday",
    "Friday",
    "Saturday",
    "Sunday"};

static const char *month_of_year[] = {
    "Unknown",
    "January",
    "February",
    "March",
    "April",
    "May",
    "June",
    "July",
    "August",
    "September",
    "October",
    "November",
    "December"};

void bt::current_time_print(bt_cts_current_time *current_time)
{
  LOG_DBG("\tDay of week   %s", day_of_week[current_time->exact_time_256.day_of_week]);

  if (current_time->exact_time_256.day == 0)
  {
    LOG_DBG("\tDay of month  Unknown");
  }
  else
  {
    LOG_DBG("\tDay of month  %u", current_time->exact_time_256.day);
  }

  LOG_DBG("\tMonth of year %s", month_of_year[current_time->exact_time_256.month]);

  if (current_time->exact_time_256.year == 0)
  {
    LOG_DBG("\tYear          Unknown");
  }
  else
  {
    LOG_DBG("\tYear          %u", current_time->exact_time_256.year);
  }
  LOG_DBG("\tTime:");
  LOG_DBG("\tHours     %u", current_time->exact_time_256.hours);
  LOG_DBG("\tMinutes   %u", current_time->exact_time_256.minutes);
  LOG_DBG("\tSeconds   %u", current_time->exact_time_256.seconds);

  LOG_DBG("\tAdjust reason:");
  LOG_DBG("\tDaylight savings %x", current_time->adjust_reason.change_of_daylight_savings_time);
  LOG_DBG("\tTime zone        %x", current_time->adjust_reason.change_of_time_zone);
  LOG_DBG("\tExternal update  %x", current_time->adjust_reason.external_reference_time_update);
  LOG_DBG("\tManual update    %x", current_time->adjust_reason.manual_time_update);
}