#include "ble/services/gadgetbridge.hpp"
#include "ble/services/gadgetbridge/gb_parse.hpp"
#include "ble/services/gadgetbridge/st_parse.hpp"

#include "ble/nus.hpp"

#include <zephyr/logging/log.h>

#include <array>
#include <string_view>

LOG_MODULE_REGISTER(gadgetbridge, CONFIG_NRF_TEST_LOG_LEVEL);

using namespace std::string_view_literals;

constexpr auto MAX_RECV_LEN = 1000;
std::array<uint8_t, MAX_RECV_LEN> recv_buf;
size_t recv_pos;

enum State
{
  None,
  Consume,
  Done,
} state;

void parse(std::string_view sv)
{
  if (sv.starts_with("GB("))
  {
    bt::services::gadgetbridge::gb_parse(sv);
  }
  else if (sv.starts_with("setTime("))
  {
    bt::services::gadgetbridge::st_parse(sv);
  }
}

void consume(const uint8_t *data, uint16_t len)
{
  if (len == 0)
  {
    LOG_ERR("Parsing error: Received empty packet");
    return;
  }

  auto sv = std::string_view(reinterpret_cast<const char *>(data), len);
  // new command
  if (sv.front() == '\u0010')
  {
    sv = sv.substr(1);
    if (state != None)
    {
      LOG_ERR("Parsing error: Received new message before end of previous was found");
    }
    recv_buf.fill(0);
    recv_pos = 0;
    if (sv.starts_with("GB(") || sv.starts_with("setTime("))
    {
      state = Consume;
    }
    else
    {
      LOG_HEXDUMP_ERR(data, len, "Parsing error: Recieved unknown command:");
    }
  }

  switch (state)
  {
  default:
    LOG_HEXDUMP_ERR(data, len, "Parsing error: Received unknown packet:");
    break;
  case Consume:
    if (sv.back() == '\n')
    {
      sv = sv.substr(0, sv.size() - 1);
      state = Done;
    }
    if (recv_pos + sv.size() > MAX_RECV_LEN)
    {
      LOG_ERR("Parsing error: Data does not fit in MAX_RECV_LEN");
      state = None;
      break;
    }
    std::copy(sv.begin(), sv.end(), &recv_buf[recv_pos]);
    recv_pos += sv.size();
    break;
  }
  if (state == Done)
  {
    parse(std::string_view(reinterpret_cast<char *>(recv_buf.begin()), recv_pos));
    state = None;
  }
}

bt::nus::nus_cb gb_nus_cb = {
    .receive = consume,
};
void bt::services::gadgetbridge::init()
{
  bt::nus::set_callback(&gb_nus_cb);
}

int bt::services::gadgetbridge::send_ver()
{
  constexpr std::string_view sv("{\"t\":\"ver\","
                                "\"fw\":\"" GIT_HASH "\","
                                "\"hw\":\"" CONFIG_BT_DIS_HW_REV_STR "\"}");
  return bt::nus::send(reinterpret_cast<const uint8_t *>(sv.data()), sv.length());
}