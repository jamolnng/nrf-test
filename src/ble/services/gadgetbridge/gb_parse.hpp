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

    MessageType str_to_type(std::string_view sv);
    void gb_parse(std::string_view sv);
  }
}