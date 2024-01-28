#pragma once

#include <cstdint>

namespace bt::auth
{
  struct auth_cb
  {
    void (*passkey_display)(unsigned int passkey);
    void (*pairing_complete)();
  };
  int init();
  void set_pairable(bool pairable);
  bool pairable();
  void set_callback(auth_cb *cb);
}