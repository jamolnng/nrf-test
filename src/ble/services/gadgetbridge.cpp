#include "ble/services/gadgetbridge.hpp"

#include "ble/nus.hpp"

#include <zephyr/sys/base64.h>
#include <zephyr/logging/log.h>
#include <zephyr/data/json.h>
#include <zephyr/types.h>

#include <array>
#include <string_view>
#include <map>

LOG_MODULE_REGISTER(gadgetbridge, CONFIG_NRF_TEST_LOG_LEVEL);

using namespace std::string_view_literals;

constexpr auto MAX_RECV_LEN = 1000;
std::array<uint8_t, MAX_RECV_LEN> recv_buf;
size_t recv_pos;

struct message_type
{
  json_obj_token type;
  static const std::array<json_obj_descr, 1> desc;
};
const std::array<json_obj_descr, 1> message_type::desc = {
    json_obj_descr JSON_OBJ_DESCR_PRIM_NAMED(message_type, "t", type, JSON_TOK_OPAQUE),
};

struct notify
{
  long id;
  json_obj_token title;
  json_obj_token subject;
  json_obj_token body;
  json_obj_token sender;
  json_obj_token tel;

  static constexpr std::string_view type = "notify"sv;
  static const std::array<json_obj_descr, 6> desc;
};
const std::array<json_obj_descr, 6> notify::desc = {
    json_obj_descr JSON_OBJ_DESCR_PRIM(notify, id, JSON_TOK_NUMBER),
    json_obj_descr JSON_OBJ_DESCR_PRIM(notify, title, JSON_TOK_OPAQUE),
    json_obj_descr JSON_OBJ_DESCR_PRIM(notify, subject, JSON_TOK_OPAQUE),
    json_obj_descr JSON_OBJ_DESCR_PRIM(notify, body, JSON_TOK_OPAQUE),
    json_obj_descr JSON_OBJ_DESCR_PRIM(notify, sender, JSON_TOK_OPAQUE),
    json_obj_descr JSON_OBJ_DESCR_PRIM(notify, tel, JSON_TOK_OPAQUE),
};

struct notify_remove
{
  long id;

  static constexpr std::string_view type = "notify-"sv;
  static const std::array<json_obj_descr, 1> desc;
};
const std::array<json_obj_descr, 1> notify_remove::desc = {
    json_obj_descr JSON_OBJ_DESCR_PRIM(notify_remove, id, JSON_TOK_NUMBER),
};

struct call
{
  json_obj_token cmd;
  json_obj_token name;
  json_obj_token number;

  static constexpr std::string_view type = "call"sv;
  static const std::array<json_obj_descr, 3> desc;
};
const std::array<json_obj_descr, 3> call::desc = {
    json_obj_descr JSON_OBJ_DESCR_PRIM(call, cmd, JSON_TOK_OPAQUE),
    json_obj_descr JSON_OBJ_DESCR_PRIM(call, name, JSON_TOK_OPAQUE),
    json_obj_descr JSON_OBJ_DESCR_PRIM(call, number, JSON_TOK_OPAQUE),
};

struct http_resp
{
  json_obj_token id;
  json_obj_token resp;
  json_obj_token err;

  static constexpr std::string_view type = "http"sv;
  static const std::array<json_obj_descr, 3> desc;
};
const std::array<json_obj_descr, 3> http_resp::desc = {
    json_obj_descr JSON_OBJ_DESCR_PRIM(http_resp, id, JSON_TOK_OPAQUE),
    json_obj_descr JSON_OBJ_DESCR_PRIM(http_resp, resp, JSON_TOK_OPAQUE),
    json_obj_descr JSON_OBJ_DESCR_PRIM(http_resp, err, JSON_TOK_OPAQUE),
};

struct musicinfo
{
  json_obj_token artist;
  json_obj_token album;
  json_obj_token track;
  int duration;
  int track_count;
  int track_number;

  static constexpr std::string_view type = "musicinfo"sv;
  static const std::array<json_obj_descr, 6> desc;
};
const std::array<json_obj_descr, 6> musicinfo::desc = {
    json_obj_descr JSON_OBJ_DESCR_PRIM(musicinfo, artist, JSON_TOK_OPAQUE),
    json_obj_descr JSON_OBJ_DESCR_PRIM(musicinfo, album, JSON_TOK_OPAQUE),
    json_obj_descr JSON_OBJ_DESCR_PRIM(musicinfo, track, JSON_TOK_OPAQUE),
    json_obj_descr JSON_OBJ_DESCR_PRIM_NAMED(musicinfo, "dur", duration, JSON_TOK_NUMBER),
    json_obj_descr JSON_OBJ_DESCR_PRIM_NAMED(musicinfo, "c", track_count, JSON_TOK_NUMBER),
    json_obj_descr JSON_OBJ_DESCR_PRIM_NAMED(musicinfo, "n", track_number, JSON_TOK_NUMBER),
};

struct musicstate
{
  json_obj_token state;
  int position;
  int shuffle;
  int repeat;

  static constexpr std::string_view type = "musicstate"sv;
  static const std::array<json_obj_descr, 4> desc;
};
const std::array<json_obj_descr, 4> musicstate::desc = {
    json_obj_descr JSON_OBJ_DESCR_PRIM(musicstate, state, JSON_TOK_OPAQUE),
    json_obj_descr JSON_OBJ_DESCR_PRIM(musicstate, position, JSON_TOK_NUMBER),
    json_obj_descr JSON_OBJ_DESCR_PRIM(musicstate, shuffle, JSON_TOK_NUMBER),
    json_obj_descr JSON_OBJ_DESCR_PRIM(musicstate, repeat, JSON_TOK_NUMBER),
};

enum State
{
  None,
  Consume,
  Done,
} state;

enum Type
{
  Unknown,
  Notify,        // notify
  NotifyRemove,  // notify-
  Call,          // call
  Weather,       // weather
  MusicInfo,     // musicinfo
  MusicState,    // musicstate
  Http,          // http
  Alarm,         // alarm
  Find,          // find
  ActivityFetch, // actfetch
  IsGPSActive,   // is_gps_active
  Vibrate,       // vibrate
  Navigation,    // nav
};

const std::map<std::string_view, Type> type_map = {
    {notify::type, Notify},
    {notify_remove::type, NotifyRemove},
    {call::type, Call},
    {musicinfo::type, MusicInfo},
    {musicstate::type, MusicState},
    {http_resp::type, Http},
};

Type str_to_type(std::string_view sv)
{
  auto t = type_map.find(sv);
  if (t != type_map.end())
    return t->second;
  return Unknown;
}

template <typename T>
int from_json(std::string_view sv, T *t)
{
  return json_obj_parse(const_cast<char *>(sv.data()),
                        sv.size(),
                        T::desc.data(),
                        T::desc.size(),
                        t);
}

void dump_notify(std::string_view sv)
{
  notify notif;
  int ret = from_json(sv, &notif);
  if (ret < 0)
    LOG_ERR("JSON parse error: %d", ret);
  else
  {
    LOG_DBG("Notify:");
    if (ret & 0b000001)
      LOG_DBG("     ID: %ld", notif.id);
    if (ret & 0b000010)
      LOG_DBG("  Title: %.*s", notif.title.length, notif.title.start);
    if (ret & 0b000100)
      LOG_DBG("Subject: %.*s", notif.subject.length, notif.subject.start);
    if (ret & 0b001000)
      LOG_DBG("   Body: %.*s", notif.body.length, notif.body.start);
    if (ret & 0b010000)
      LOG_DBG(" Sender: %.*s", notif.sender.length, notif.sender.start);
    if (ret & 0b100000)
      LOG_DBG("    Tel: %.*s", notif.tel.length, notif.tel.start);
  }
}

void dump_notify_remove(std::string_view sv)
{
  notify_remove notif;
  int ret = from_json(sv, &notif);
  if (ret < 0)
    LOG_ERR("JSON parse error: %d", ret);
  else
  {
    LOG_DBG("Notify Remove:");
    if (ret & 0b1)
      LOG_DBG("ID: %ld", notif.id);
  }
}

void dump_call(std::string_view sv)
{
  call c;
  int ret = from_json(sv, &c);
  if (ret < 0)
    LOG_ERR("JSON parse error: %d", ret);
  else
  {
    LOG_DBG("Call:");
    if (ret & 0b001)
      LOG_DBG("   CMD: %.*s", c.cmd.length, c.cmd.start);
    if (ret & 0b010)
      LOG_DBG("  Name: %.*s", c.name.length, c.name.start);
    if (ret & 0b100)
      LOG_DBG("Number: %.*s", c.number.length, c.number.start);
  }
}

void dump_http_resp(std::string_view sv)
{
  http_resp resp;
  int ret = from_json(sv, &resp);
  if (ret < 0)
    LOG_ERR("JSON parse error: %d", ret);
  else
  {
    LOG_DBG("HTTP Response:");
    if (ret & 0b001)
      LOG_DBG("  ID: %.*s", resp.id.length, resp.id.start);
    if (ret & 0b010)
      LOG_DBG("Resp: %.*s", resp.resp.length, resp.resp.start);
    if (ret & 0b100)
      LOG_DBG(" Err: %.*s", resp.err.length, resp.err.start);
  }
}

void dump_musicinfo(std::string_view sv)
{
  musicinfo info;
  int ret = from_json(sv, &info);
  if (ret < 0)
    LOG_ERR("JSON parse error: %d", ret);
  else
  {
    LOG_DBG("Music Info:");
    if (ret & 0b000001)
      LOG_DBG("      Artist: %.*s", info.artist.length, info.artist.start);
    if (ret & 0b000010)
      LOG_DBG("       Album: %.*s", info.album.length, info.album.start);
    if (ret & 0b000100)
      LOG_DBG("       Track: %.*s", info.track.length, info.track.start);
    if (ret & 0b001000)
      LOG_DBG("    Duration: %d", info.duration);
    if (ret & 0b010000)
      LOG_DBG(" Track count: %d", info.track_count);
    if (ret & 0b100000)
      LOG_DBG("Track number: %d", info.track_number);
  }
}

void dump_musicstate(std::string_view sv)
{
  musicstate state;
  int ret = from_json(sv, &state);
  if (ret < 0)
    LOG_ERR("JSON parse error: %d", ret);
  else
  {
    LOG_DBG("Music State:");
    if (ret & 0b0001)
      LOG_DBG("   State: %.*s", state.state.length, state.state.start);
    if (ret & 0b0010)
      LOG_DBG("Position: %d", state.position);
    if (ret & 0b0100)
      LOG_DBG(" Shuffle: %d", state.shuffle);
    if (ret & 0b1000)
      LOG_DBG("  Repeat: %d", state.repeat);
  }
}

void base64_decode_in_place(std::string_view sv)
{
  auto end = sv.find(')');
  auto decode = sv.substr(6, end - 7);
  size_t decoded_length;
  auto *out = reinterpret_cast<uint8_t *>(const_cast<char *>(sv.substr(1).data()));
  base64_decode(out,
                end,
                &decoded_length,
                reinterpret_cast<uint8_t *>(const_cast<char *>(decode.data())),
                decode.size());
  // json_escape(reinterpret_cast<char *>(out), &decoded_length, end);
  memset(&out[decoded_length + 1], ' ', end - decoded_length - 1);
  out[-1] = '"';
  out[decoded_length] = '"';
}

void dump_gb(std::string_view sv)
{
  size_t idx;
  while ((idx = sv.find("atob(\""sv)) != std::string_view::npos)
  {
    base64_decode_in_place(sv.substr(idx));
  }
  message_type base;
  int ret = from_json(sv, &base);
  auto type_str = std::string_view(base.type.start, base.type.length);
  if (ret < 0)
  {
    LOG_ERR("JSON decode error: %d", ret);
    return;
  }
  switch (str_to_type(type_str))
  {
  case Notify:
    dump_notify(sv);
    break;
  case NotifyRemove:
    dump_notify_remove(sv);
    break;
  case Call:
    dump_call(sv);
    break;
  case Http:
    dump_http_resp(sv);
    break;
  case MusicInfo:
    dump_musicinfo(sv);
    break;
  case MusicState:
    dump_musicstate(sv);
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
    sv = sv.substr(3, sv.size() - 4);
    dump_gb(sv);
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
  constexpr std::string_view sv("{\"t\":\"ver\","
                                "\"fw\":\"" CONFIG_BT_DIS_FW_REV_STR "\","
                                "\"hw\":\"" CONFIG_BT_DIS_HW_REV_STR "\"}");
  return bt::nus::send(reinterpret_cast<const uint8_t *>(sv.data()), sv.length());
}