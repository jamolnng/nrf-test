#pragma once

#include <zephyr/data/json.h>

#include <array>
#include <string_view>

namespace services
{
  namespace gadgetbridge
  {
    enum MessageType
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

      static constexpr std::string_view type = "notify";
      static const std::array<json_obj_descr, 6> desc;
    };

    struct notify_remove_message
    {
      long id;

      static constexpr std::string_view type = "notify-";
      static const std::array<json_obj_descr, 1> desc;
    };

    struct call_message
    {
      json_obj_token cmd;
      json_obj_token name;
      json_obj_token number;

      static constexpr std::string_view type = "call";
      static const std::array<json_obj_descr, 3> desc;
    };

    struct http_message
    {
      json_obj_token id;
      json_obj_token resp;
      json_obj_token err;

      static constexpr std::string_view type = "http";
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

      static constexpr std::string_view type = "musicinfo";
      static const std::array<json_obj_descr, 6> desc;
    };

    struct musicstate_message
    {
      json_obj_token state;
      int position;
      int shuffle;
      int repeat;

      static constexpr std::string_view type = "musicstate";
      static const std::array<json_obj_descr, 4> desc;
    };

    MessageType str_to_type(std::string_view sv);
    void gb_parse(std::string_view sv);
  }
}