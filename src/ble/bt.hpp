#include <cstdint>

namespace bt
{
  // struct bt_cb
  // {
  // void (*connected)();
  // };

  int init();
  uint32_t max_send_len();
  bool connected();
  bool secure_connection();

  // void set_callback();
}