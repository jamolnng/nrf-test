#include "system/gadgetbridge/gb.hpp"

#include "ble/nus.hpp"

#include <zephyr/logging/log.h>

#include <string_view>
#include <array>

using namespace std::string_view_literals;

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

enum Type
{
  Unknown,
  Notify,
  NotifyRemove,
  Weather,
  MusicInfo,
  MusicState,
  Http,
  IsGPSActive,
};

static std::array<char, 100> key_tmp; // string views dont require null terminator so we can save us some stack allocation
std::string_view extract_value_str(std::string_view key, std::string_view data)
{
  key_tmp[0] = '"';
  std::copy(key.begin(), key.end(), &key_tmp[1]);
  key_tmp[key.size() + 1] = '"';
  key_tmp[key.size() + 2] = ':';
  key_tmp[key.size() + 3] = '"';

  key = std::string_view(key_tmp.data(), key.size() + 4);

  size_t idx;
  if ((idx = data.find(key)) != std::string_view::npos)
  {
    size_t start = idx, len = 0;
    auto search = data.substr(idx + key.size());
    while ((idx = search.find('"')) != std::string_view::npos)
    {
      if (idx == 0)
      {
        break;
      }
      len += idx;
      if (search[idx - 1] != '\\')
      {
        break;
      }
      else
      {
        len++;
      }
      search = search.substr(idx + 1);
    }
    return data.substr(start + key.size()).substr(0, len);
  }
  return std::string_view{};
}

Type str_to_type(std::string_view sv)
{
  if (sv == "notify"sv)
  {
    return Notify;
  }
  if (sv == "notify-"sv)
  {
    return NotifyRemove;
  }
  if (sv == "weather"sv)
  {
    return Weather;
  }
  if (sv == "musicinfo"sv)
  {
    return MusicInfo;
  }
  if (sv == "musicstate"sv)
  {
    return MusicState;
  }
  if (sv == "http"sv)
  {
    return Http;
  }
  if (sv == "is_gps_active"sv)
  {
    return IsGPSActive;
  }
  return Unknown;
}

void dump_gb(std::string_view sv)
{
  auto v = extract_value_str("t"sv, sv);
  LOG_DBG("      t: %.*s", v.size(), v.data());
  switch (str_to_type(extract_value_str("t"sv, sv)))
  {
  case Notify:
    // v = extract_value_long("id"sv, sv);
    // LOG_DBG("     id: %.*s".v.size(), v.data());
    v = extract_value_str("src"sv, sv);
    LOG_DBG("    src: %.*s", v.size(), v.data());
    v = extract_value_str("title"sv, sv);
    LOG_DBG("  title: %.*s", v.size(), v.data());
    v = extract_value_str("subject"sv, sv);
    LOG_DBG("subject: %.*s", v.size(), v.data());
    v = extract_value_str("body"sv, sv);
    LOG_DBG("   body: %.*s", v.size(), v.data());
    v = extract_value_str("sender"sv, sv);
    LOG_DBG(" sender: %.*s", v.size(), v.data());
    break;
  case NotifyRemove:
    // v = extract_value_long("id"sv, sv);
    // LOG_DBG("     id: %.*s".v.size(), v.data());
    break;
  case Weather:
    LOG_DBG("%.*s", sv.size(), sv.data());
    break;
  case MusicInfo:
    LOG_DBG("%.*s", sv.size(), sv.data());
    break;
  case MusicState:
    LOG_DBG("%.*s", sv.size(), sv.data());
    break;
  case Http:
    v = extract_value_str("resp"sv, sv);
    LOG_DBG("   resp: %.*s", v.size(), v.data());
    v = extract_value_str("err"sv, sv);
    LOG_DBG("    err: %.*s", v.size(), v.data());
    break;
  case IsGPSActive:
    break;
  default:
    LOG_DBG("%.*s", sv.size(), sv.data());
    break;
  }
}

void parse(std::string_view sv)
{
  if (sv.starts_with("GB("))
  {
    dump_gb(sv);
    // sv = sv.substr(4, sv.size() - 6);
    // LOG_DBG("GB: %.*s", sv.size(), sv.data());
    // auto skip = sizeof("\"t\":\"") - 1;
    // switch (str_to_type(extract_value_str("t"sv, sv)))
    // {
    // case Notify:
    //   LOG_DBG("Notify");
    //   break;
    // case NotifyRemove:
    //   LOG_DBG("NotifyRemove");
    //   break;
    // case Weather:
    //   LOG_DBG("Weather");
    //   break;
    // case MusicInfo:
    //   LOG_DBG("MusicInfo");
    //   break;
    // case MusicState:
    //   LOG_DBG("MusicState");
    //   break;
    // case Http:
    //   LOG_DBG("Http");
    //   break;
    // case IsGPSActive:
    //   LOG_DBG("IsGPSActive");
    //   break;
    // default:
    //   LOG_DBG("Unknown");
    //   break;
    // }
  }
  else if (sv.starts_with("setTime("))
  {
    sv = sv.substr(7);
    LOG_DBG("ST: %.*s", sv.size(), sv.data());
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

bt::nus::nus_cb cb;
void system::gadgetbridge::init()
{
  cb.receive = consume;
  bt::nus::set_callback(&cb);
}

int system::gadgetbridge::send_ver()
{
  std::string_view sv("{\"t\":\"ver\","
                      "\"fw\":\"" CONFIG_BT_DIS_FW_REV_STR "\","
                      "\"hw\":\"" CONFIG_BT_DIS_HW_REV_STR "\"}");
  return bt::nus::send(reinterpret_cast<const uint8_t *>(sv.data()), sv.length());
}