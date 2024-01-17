#include "system/gadgetbridge/gb.hpp"

#include "ble/nus.hpp"

#include <zephyr/logging/log.h>

#include <string_view>
#include <array>

constexpr auto MAX_RECV_LEN = 1000;
std::array<uint8_t, MAX_RECV_LEN> recv_buf;
size_t recv_pos;

LOG_MODULE_REGISTER(gadgetbridge, CONFIG_NRF_TEST_LOG_LEVEL);

enum State
{
  None,
  Consume,
  Done,
} state;

void parse(std::string_view sv)
{
  LOG_DBG("%.*s", sv.size(), sv.data());
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
    size_t idx;
    if ((idx = sv.find("GB(")) != std::string_view::npos)
    {
      state = Consume;
    }
    else if ((idx = sv.find("setTime(")) != std::string_view::npos)
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

bt::nus::nus_cb cb;
void system::gadgetbridge::init()
{
  cb.receive = consume;
  bt::nus::set_callback(&cb);
}