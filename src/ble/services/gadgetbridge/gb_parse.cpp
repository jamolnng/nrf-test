#include "ble/services/gadgetbridge/gb_parse.hpp"

#include <zephyr/logging/log.h>
#include <zephyr/sys/base64.h>

#include <map>

using namespace std::string_view_literals;

LOG_MODULE_REGISTER(gadgetbridge_gb_parse, CONFIG_NRF_TEST_LOG_LEVEL);

using namespace services::gadgetbridge;

struct message_type
{
  json_obj_token type;
  static const std::array<json_obj_descr, 1> desc;
};

struct notify_message
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

struct notify_remove_message
{
  long id;

  static constexpr std::string_view type = "notify-"sv;
  static const std::array<json_obj_descr, 1> desc;
};

struct call_message
{
  json_obj_token cmd;
  json_obj_token name;
  json_obj_token number;

  static constexpr std::string_view type = "call"sv;
  static const std::array<json_obj_descr, 3> desc;
};

struct http_message
{
  json_obj_token id;
  json_obj_token resp;
  json_obj_token err;

  static constexpr std::string_view type = "http"sv;
  static const std::array<json_obj_descr, 3> desc;
};

struct musicinfo_message
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

struct musicstate_message
{
  json_obj_token state;
  int position;
  int shuffle;
  int repeat;

  static constexpr std::string_view type = "musicstate"sv;
  static const std::array<json_obj_descr, 4> desc;
};

const std::array<json_obj_descr, 1> message_type::desc = {
    json_obj_descr JSON_OBJ_DESCR_PRIM_NAMED(message_type, "t", type, JSON_TOK_OPAQUE),
};
const std::array<json_obj_descr, 6> notify_message::desc = {
    json_obj_descr JSON_OBJ_DESCR_PRIM(notify_message, id, JSON_TOK_NUMBER),
    json_obj_descr JSON_OBJ_DESCR_PRIM(notify_message, title, JSON_TOK_OPAQUE),
    json_obj_descr JSON_OBJ_DESCR_PRIM(notify_message, subject, JSON_TOK_OPAQUE),
    json_obj_descr JSON_OBJ_DESCR_PRIM(notify_message, body, JSON_TOK_OPAQUE),
    json_obj_descr JSON_OBJ_DESCR_PRIM(notify_message, sender, JSON_TOK_OPAQUE),
    json_obj_descr JSON_OBJ_DESCR_PRIM(notify_message, tel, JSON_TOK_OPAQUE),
};
const std::array<json_obj_descr, 1> notify_remove_message::desc = {
    json_obj_descr JSON_OBJ_DESCR_PRIM(notify_remove_message, id, JSON_TOK_NUMBER),
};
const std::array<json_obj_descr, 3> call_message::desc = {
    json_obj_descr JSON_OBJ_DESCR_PRIM(call_message, cmd, JSON_TOK_OPAQUE),
    json_obj_descr JSON_OBJ_DESCR_PRIM(call_message, name, JSON_TOK_OPAQUE),
    json_obj_descr JSON_OBJ_DESCR_PRIM(call_message, number, JSON_TOK_OPAQUE),
};
const std::array<json_obj_descr, 3> http_message::desc = {
    json_obj_descr JSON_OBJ_DESCR_PRIM(http_message, id, JSON_TOK_OPAQUE),
    json_obj_descr JSON_OBJ_DESCR_PRIM(http_message, resp, JSON_TOK_OPAQUE),
    json_obj_descr JSON_OBJ_DESCR_PRIM(http_message, err, JSON_TOK_OPAQUE),
};
const std::array<json_obj_descr, 6> musicinfo_message::desc = {
    json_obj_descr JSON_OBJ_DESCR_PRIM(musicinfo_message, artist, JSON_TOK_OPAQUE),
    json_obj_descr JSON_OBJ_DESCR_PRIM(musicinfo_message, album, JSON_TOK_OPAQUE),
    json_obj_descr JSON_OBJ_DESCR_PRIM(musicinfo_message, track, JSON_TOK_OPAQUE),
    json_obj_descr JSON_OBJ_DESCR_PRIM_NAMED(musicinfo_message, "dur", duration, JSON_TOK_NUMBER),
    json_obj_descr JSON_OBJ_DESCR_PRIM_NAMED(musicinfo_message, "c", track_count, JSON_TOK_NUMBER),
    json_obj_descr JSON_OBJ_DESCR_PRIM_NAMED(musicinfo_message, "n", track_number, JSON_TOK_NUMBER),
};
const std::array<json_obj_descr, 4> musicstate_message::desc = {
    json_obj_descr JSON_OBJ_DESCR_PRIM(musicstate_message, state, JSON_TOK_OPAQUE),
    json_obj_descr JSON_OBJ_DESCR_PRIM(musicstate_message, position, JSON_TOK_NUMBER),
    json_obj_descr JSON_OBJ_DESCR_PRIM(musicstate_message, shuffle, JSON_TOK_NUMBER),
    json_obj_descr JSON_OBJ_DESCR_PRIM(musicstate_message, repeat, JSON_TOK_NUMBER),
};

const std::map<std::string_view, MessageType> type_map = {
    {notify_message::type, Notify},
    {notify_remove_message::type, NotifyRemove},
    {call_message::type, Call},
    {musicinfo_message::type, MusicInfo},
    {musicstate_message::type, MusicState},
    {http_message::type, Http},
};

MessageType services::gadgetbridge::str_to_type(std::string_view sv)
{
  auto t = type_map.find(sv);
  if (t != type_map.end())
    return t->second;
  return Unknown;
}

template <typename T>
int from_json(std::string_view sv, T *t)
{
  return json_obj_parse(const_cast<char *>(sv.data()), sv.size(),
                        T::desc.data(), T::desc.size(),
                        t);
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

void dump_notify(std::string_view sv)
{
  notify_message notif;
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
  notify_remove_message notif;
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
  call_message c;
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
  http_message resp;
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
  musicinfo_message info;
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
  musicstate_message state;
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
  switch (services::gadgetbridge::str_to_type(type_str))
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

void services::gadgetbridge::gb_parse(std::string_view sv)
{
  sv = sv.substr(3, sv.size() - 4);
  dump_gb(sv);
}