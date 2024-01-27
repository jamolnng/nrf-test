#include "ble/services/gadgetbridge/st_parse.hpp"

#include <zephyr/logging/log.h>
#include <zephyr/posix/time.h>

#include <array>
#include <charconv>
#include <chrono>

LOG_MODULE_REGISTER(gadgetbridge_st_parse, CONFIG_NRF_TEST_LOG_LEVEL);

void services::gadgetbridge::st_parse(std::string_view sv)
{
  std::array<char, 25> time_buf{0};
  auto now = std::time(nullptr);
  if (std::strftime(time_buf.data(), time_buf.size(), "%c", std::localtime(&now)))
    LOG_DBG("Old time %s", time_buf.data());

  sv = sv.substr(sv.find("setTime(") + 8);
  auto end = sv.find(')');
  auto seconds_str = sv.substr(0, end);

  long seconds;
  auto result = std::from_chars(seconds_str.data(), seconds_str.end(), seconds);
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

  int tz_offset;
  result = std::from_chars(tz_offset_str.data(), tz_offset_str.end(), tz_offset);
  if (result.ec == std::errc::invalid_argument)
    return;

  std::array<char, std::size("UTC+00")> tz_buf{0};
  snprintf(tz_buf.data(), tz_buf.size(), "UTC%+03d", -tz_offset % 100);
  setenv("TZ", tz_buf.data(), 1);
  tzset();

  now = std::time(nullptr);
  if (std::strftime(time_buf.data(), time_buf.size(), "%c", std::localtime(&now)))
    LOG_DBG("New time %s", time_buf.data());
}