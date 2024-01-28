#pragma once

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/pm/device.h>
#include <zephyr/drivers/pwm.h>

#include <cstdint>

namespace managers::display
{
  class Display
  {
    enum State
    {
      On,
      Sleep,
      // Off, // requires external regulator
    };

  public:
    Display(Display const &) = delete;
    void operator=(Display const &) = delete;

    static Display &instance();
    void init();
    void on();
    void sleep();
    // void off(); // requires external regulator

    void set_brightness(uint8_t brightness);
    uint8_t get_brightness();

  private:
    Display(const device *display, const device *touch, const pwm_dt_spec backlight);
    ~Display() = default;
    const device *_display;
    const device *_touch;
    const pwm_dt_spec _backlight;
    uint8_t _brightness{32}, _last_brightness{32};
    State _state{Sleep};

    static void render(k_work *work);
    K_WORK_DELAYABLE_DEFINE(_render_work, render);
    k_work_sync _cancel_sync;
  };
}