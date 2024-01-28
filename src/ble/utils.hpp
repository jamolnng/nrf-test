#pragma once

#include <bluetooth/services/cts_client.h>

namespace bt::utils
{
  void current_time_print(bt_cts_current_time *current_time);
}