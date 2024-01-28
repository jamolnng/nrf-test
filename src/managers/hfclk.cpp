#include <zephyr/init.h>
#include <nrfx_clock.h>

int hfclk()
{
  return nrfx_clock_divider_set(NRF_CLOCK_DOMAIN_HFCLK, NRF_CLOCK_HFCLK_DIV_1);
}
SYS_INIT(hfclk, EARLY, 0);