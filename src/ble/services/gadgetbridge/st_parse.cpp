#include "ble/services/gadgetbridge/st_parse.hpp"

#include <zephyr/logging/log.h>
#include <zephyr/posix/time.h>

#include <charconv>
#include <chrono>

LOG_MODULE_REGISTER(gadgetbridge_st_parse, CONFIG_NRF_TEST_LOG_LEVEL);

void services::gadgetbridge::st_parse(std::string_view sv)
{
  char time_str[40] = {'\0'};
  auto now = std::time(nullptr);
  if (std::strftime(time_str, std::size(time_str), "%c", std::localtime(&now)))
    LOG_DBG("Old time %s", time_str);

  sv = sv.substr(sv.find("setTime(") + 8);
  auto end = sv.find(')');
  auto seconds_str = sv.substr(0, end);

  long seconds;
  auto result = std::from_chars(seconds_str.data(), seconds_str.data() + seconds_str.size(), seconds);
  if (result.ec == std::errc::invalid_argument)
    return;

  timespec tspec = {
      .tv_sec = seconds,
      .tv_nsec = 0,
  };
  clock_settime(CLOCK_REALTIME, &tspec);

  sv = sv.substr(sv.find("setTimeZone(") + 12);
  end = sv.find(')');
  auto tz_offset_str = sv.substr(0, end);

  float tz_offset;
  result = std::from_chars(tz_offset_str.data(), tz_offset_str.data() + tz_offset_str.size(), tz_offset);
  if (result.ec == std::errc::invalid_argument)
    return;

  char tz_c[std::size("UTC+00")] = {'\0'};
  snprintf(tz_c, sizeof(tz_c), "UTC%+03d", -int(tz_offset));
  setenv("TZ", tz_c, 1);
  tzset();

  now = std::time(nullptr);
  if (std::strftime(time_str, std::size(time_str), "%c", std::localtime(&now)))
    LOG_DBG("New time %s", time_str);
}