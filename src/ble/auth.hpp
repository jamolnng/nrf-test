#include <cstdint>

namespace bt
{
  namespace auth
  {
    struct auth_cb
    {
      void (*passkey_display)(unsigned int passkey);
    };
    int init();
    void set_pairable(bool pairable);
    bool pairable();
    void set_callback(auth_cb *cb);
  }
}