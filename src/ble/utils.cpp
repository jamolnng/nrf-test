#include "ble/utils.hpp"

#include <zephyr/logging/log.h>

#include <array>
#include <string_view>

using namespace std::string_view_literals;

LOG_MODULE_REGISTER(bt_utils, CONFIG_NRF_TEST_LOG_LEVEL);

constexpr std::array<std::string_view, 8> day_of_week = {
    "Unknown"sv,
    "Monday"sv,
    "Tuesday"sv,
    "Wednesday"sv,
    "Thursday"sv,
    "Friday"sv,
    "Saturday"sv,
    "Sunday"sv,
};

constexpr std::array<std::string_view, 13> month_of_year = {
    "Unknown"sv,
    "January"sv,
    "February"sv,
    "March"sv,
    "April"sv,
    "May"sv,
    "June"sv,
    "July"sv,
    "August"sv,
    "September"sv,
    "October"sv,
    "November"sv,
    "December"sv,
};

void bt::current_time_print(bt_cts_current_time *current_time)
{
  auto day = day_of_week[current_time->exact_time_256.day_of_week];
  LOG_DBG("\tDay of week   %.*s", day.size(), day.data());

  if (current_time->exact_time_256.day == 0)
  {
    LOG_DBG("\tDay of month  Unknown");
  }
  else
  {
    LOG_DBG("\tDay of month  %u", current_time->exact_time_256.day);
  }

  auto month = month_of_year[current_time->exact_time_256.month];
  LOG_DBG("\tMonth of year %.*s", month.size(), month.data());

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