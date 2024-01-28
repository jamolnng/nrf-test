#pragma once

#include <zephyr/kernel.h>

namespace managers::devkit
{
  class DevKit
  {
  public:
    DevKit(DevKit const &) = delete;
    void operator=(DevKit const &) = delete;
    static DevKit &instance();
    void init();

  private:
    DevKit();
    ~DevKit() = default;

    static void blink(k_work *work);
    K_WORK_DELAYABLE_DEFINE(_blink_work, blink);
  };
}