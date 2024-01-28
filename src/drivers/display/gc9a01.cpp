#define DT_DRV_COMPAT buydisplay_gc9a01

#include <string.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/kernel.h>
#include <math.h>
#include <zephyr/logging/log.h>
#include <inttypes.h>
#include <zephyr/pm/pm.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/policy.h>

#include <array>

LOG_MODULE_REGISTER(gc9a01, CONFIG_DISPLAY_LOG_LEVEL);

enum DisplayCommand
{
  GC9A01A_SLPIN = 0x10,     ///< Enter Sleep Mode
  GC9A01A_SLPOUT = 0x11,    ///< Sleep Out
  GC9A01A_PTLON = 0x12,     ///< Partial Mode ON
  GC9A01A_NORON = 0x13,     ///< Normal Display Mode ON
  GC9A01A_INVOFF = 0x20,    ///< Display Inversion OFF
  GC9A01A_INVON = 0x21,     ///< Display Inversion ON
  GC9A01A_DISPOFF = 0x28,   ///< Display OFF
  GC9A01A_DISPON = 0x29,    ///< Display ON
  GC9A01A_CASET = 0x2A,     ///< Column Address Set
  GC9A01A_PASET = 0x2B,     ///< Page Address Set
  GC9A01A_RAMWR = 0x2C,     ///< Memory Write
  GC9A01A_PTLAR = 0x30,     ///< Partial Area
  GC9A01A_VSCRDEF = 0x33,   ///< Vertical Scrolling Definition
  GC9A01A_TEOFF = 0x34,     ///< Tearing effect line off
  GC9A01A_TEON = 0x35,      ///< Tearing effect line on
  GC9A01A_MADCTL = 0x36,    ///< Memory Access Control
  GC9A01A_VSCRSADD = 0x37,  ///< Vertical Scrolling Start Address
  GC9A01A_PIXFMT = 0x3A,    ///< COLMOD: Pixel Format Set
  GC9A01A1_DFUNCTR = 0xB6,  ///< Display Function Control
  GC9A01A1_VREG1A = 0xC3,   ///< Vreg1a voltage control
  GC9A01A1_VREG1B = 0xC4,   ///< Vreg1b voltage control
  GC9A01A1_VREG2A = 0xC9,   ///< Vreg2a voltage control
  GC9A01A_RDID1 = 0xDA,     ///< Read ID 1
  GC9A01A_RDID2 = 0xDB,     ///< Read ID 2
  GC9A01A_RDID3 = 0xDC,     ///< Read ID 3
  GC9A01A1_GMCTRP1 = 0xE0,  ///< Positive Gamma Correction
  GC9A01A1_GMCTRN1 = 0xE1,  ///< Negative Gamma Correction
  GC9A01A_FRAMERATE = 0xE8, ///< Frame rate control
  GC9A01A_INREGEN2 = 0xEF,  ///< Inter register enable 2
  GC9A01A_GAMMA1 = 0xF0,    ///< Set gamma 1
  GC9A01A_GAMMA2 = 0xF1,    ///< Set gamma 2
  GC9A01A_GAMMA3 = 0xF2,    ///< Set gamma 3
  GC9A01A_GAMMA4 = 0xF3,    ///< Set gamma 4
  GC9A01A_INREGEN1 = 0xFE,  ///< Inter register enable 1
  COL_ADDR_SET = 0x2A,
  ROW_ADDR_SET = 0x2B,
  MEM_WR = 0x2C,
  MEM_WR_CONT = 0x3C,
  COLOR_MODE = 0x3A,
  COLOR_MODE_12_BIT = 0x03,
  COLOR_MODE_16_BIT = 0x05,
  COLOR_MODE_18_BIT = 0x06,
  SLPIN = 0x10,
  SLPOUT = 0x11,
};

enum MemoryAccessParams
{
  RowOrderTopToBottom = 0x00,
  RowOrderBottomToTop = 0x80,

  ColumnOrderLeftToRight = 0x00,
  ColumnOrderRightToLeft = 0x40,

  ExchangeModeNormal = 0x00,
  ExchangeModeReverse = 0x20,

  VerticalRefreshTopToBottom = 0x00,
  VerticalRefreshBottomToTop = 0x10,

  PixelOrderRGB = 0x00,
  PixelOrderBGR = 0x08,

  HorizonalRefreshLeftToRight = 0x00,
  HorizonalRefreshRightToLeft = 0x40,
};

// constexpr auto MADCTL_MY = 0x80;  ///< Bottom to top
// constexpr auto MADCTL_MX = 0x40;  ///< Right to left
// constexpr auto MADCTL_MV = 0x20;  ///< Reverse Mode
// constexpr auto MADCTL_ML = 0x10;  ///< LCD refresh Bottom to top
// constexpr auto MADCTL_RGB = 0x00; ///< Red-Green-Blue pixel order
// constexpr auto MADCTL_BGR = 0x08; ///< Blue-Green-Red pixel order
// constexpr auto MADCTL_MH = 0x04;  ///< LCD refresh right to left

constexpr auto DISPLAY_WIDTH = DT_INST_PROP(0, width);
constexpr auto DISPLAY_HEIGHT = DT_INST_PROP(0, height);

struct GC9A01CMD
{
  uint8_t cmd;
  uint8_t argc;
  uint8_t *argv;
};
constexpr GC9A01CMD gc9a01_initcmds[] = {
    {GC9A01A_INREGEN2, 0},
    {0xEB, 1, (uint8_t[]){0x14}},
    {GC9A01A_INREGEN1, 0},
    {GC9A01A_INREGEN2, 0},
    {0xEB, 1, (uint8_t[]){0x14}},
    {0x84, 1, (uint8_t[]){0x40}},
    {0x85, 1, (uint8_t[]){0xFF}},
    {0x86, 1, (uint8_t[]){0xFF}},
    {0x87, 1, (uint8_t[]){0xFF}},
    {0x88, 1, (uint8_t[]){0x0A}},
    {0x89, 1, (uint8_t[]){0x21}},
    {0x8A, 1, (uint8_t[]){0x00}},
    {0x8B, 1, (uint8_t[]){0x80}},
    {0x8C, 1, (uint8_t[]){0x01}},
    {0x8D, 1, (uint8_t[]){0x01}},
    {0x8E, 1, (uint8_t[]){0xFF}},
    {0x8F, 1, (uint8_t[]){0xFF}},
    {0xB6, 2, (uint8_t[]){0x00, 0x00}},
#if DT_PROP(DT_INST(0, buydisplay_gc9a01), rotation) == 0
    {GC9A01A_MADCTL, 1, (uint8_t[]){ExchangeModeReverse | RowOrderBottomToTop | ColumnOrderRightToLeft | PixelOrderBGR}},
#elif DT_PROP(DT_INST(0, buydisplay_gc9a01), rotation) == 90
    {GC9A01A_MADCTL, 1, (uint8_t[]){HorizonalRefreshLeftToRight | RowOrderBottomToTop | 0 | PixelOrderBGR}},
#elif DT_PROP(DT_INST(0, buydisplay_gc9a01), rotation) == 180
    {GC9A01A_MADCTL, 1, (uint8_t[]){ExchangeModeReverse | 0 | 0 | PixelOrderBGR}},
#elif DT_PROP(DT_INST(0, buydisplay_gc9a01), rotation) == 270
    {GC9A01A_MADCTL, 1, (uint8_t[]){HorizonalRefreshLeftToRight | ColumnOrderRightToLeft | 0 | 0 | PixelOrderBGR}},
#else
#error "Unsupported rotation. Use 0, 90, 180 or 270."
#endif
    {GC9A01A_PIXFMT, 1, (uint8_t[]){COLOR_MODE_16_BIT}},
    {0x90, 4, (uint8_t[]){0x08, 0x08, 0x08, 0x08}},
    {0xBD, 1, (uint8_t[]){0x06}},
    {0xBC, 1, (uint8_t[]){0x00}},
    {0xFF, 3, (uint8_t[]){0x60, 0x01, 0x04}},
    {GC9A01A1_VREG1A, 1, (uint8_t[]){0x13}},
    {GC9A01A1_VREG1B, 1, (uint8_t[]){0x13}},
    {GC9A01A1_VREG2A, 1, (uint8_t[]){0x22}},
    {0xBE, 1, (uint8_t[]){0x11}},
    {GC9A01A1_GMCTRN1, 2, (uint8_t[]){0x10, 0x0E}},
    {0xDF, 3, (uint8_t[]){0x21, 0x0c, 0x02}},
    {GC9A01A_GAMMA1, 6, (uint8_t[]){0x45, 0x09, 0x08, 0x08, 0x26, 0x2A}},
    {GC9A01A_GAMMA2, 6, (uint8_t[]){0x43, 0x70, 0x72, 0x36, 0x37, 0x6F}},
    {GC9A01A_GAMMA3, 6, (uint8_t[]){0x45, 0x09, 0x08, 0x08, 0x26, 0x2A}},
    {GC9A01A_GAMMA4, 6, (uint8_t[]){0x43, 0x70, 0x72, 0x36, 0x37, 0x6F}},
    {0xED, 2, (uint8_t[]){0x1B, 0x0B}},
    {0xAE, 1, (uint8_t[]){0x77}},
    {0xCD, 1, (uint8_t[]){0x63}},
    {0x70, 9, (uint8_t[]){0x07, 0x07, 0x04, 0x0E, 0x0F, 0x09, 0x07, 0x08, 0x03}},
    {GC9A01A_FRAMERATE, 1, (uint8_t[]){0x34}},
    {0x62, 12, (uint8_t[]){0x18, 0x0D, 0x71, 0xED, 0x70, 0x70, 0x18, 0x0F, 0x71, 0xEF, 0x70, 0x70}},
    {0x63, 12, (uint8_t[]){0x18, 0x11, 0x71, 0xF1, 0x70, 0x70, 0x18, 0x13, 0x71, 0xF3, 0x70, 0x70}},
    {0x64, 7, (uint8_t[]){0x28, 0x29, 0xF1, 0x01, 0xF1, 0x00, 0x07}},
    {0x66, 10, (uint8_t[]){0x3C, 0x00, 0xCD, 0x67, 0x45, 0x45, 0x10, 0x00, 0x00, 0x00}},
    {0x67, 10, (uint8_t[]){0x00, 0x3C, 0x00, 0x00, 0x00, 0x01, 0x54, 0x10, 0x32, 0x98}},
    {0x74, 7, (uint8_t[]){0x10, 0x85, 0x80, 0x00, 0x00, 0x4E, 0x00}},
    {0x98, 2, (uint8_t[]){0x3e, 0x07}},
    {GC9A01A_TEON, 1, (uint8_t[]){GC9A01A_INVOFF}},
    {GC9A01A_INVON, 0},
    // {GC9A01A_DISPON, 0}, // Display on
    // {GC9A01A_SLPOUT, 0}, // Exit sleep
};

struct gc9a01_config_t
{
  struct spi_dt_spec bus;
  struct gpio_dt_spec dc_gpio;
  struct gpio_dt_spec bl_gpio;
  struct gpio_dt_spec reset_gpio;
};

uint16_t rgb8_to_rgb565(uint8_t r, uint8_t g, uint8_t b)
{
  uint16_t red5 = uint16_t(float(r) / 255.0f * 31.0f);
  uint16_t gre5 = uint16_t(float(g) / 255.0f * 63.0f);
  uint16_t blu5 = uint16_t(float(b) / 255.0f * 31.0f);
  return ((red5 << 11) | (gre5 << 5) | blu5);
}

int gc9a01_write_cmd(const device *dev, uint8_t cmd);
int gc9a01_write_data(const device *dev, const uint8_t *data, size_t len);
int gc9a01_write_cmd_data(const device *dev, uint8_t cmd, const uint8_t *data, size_t len);
void gc9a01_set_frame(const device *dev,
                      const uint16_t x, const uint16_t y,
                      const uint16_t endx, const uint16_t endy);

// these are macros so the __ASSERT macro picks up the correct line of code
#define gc9a01_spi_resume(dev)                                                \
  {                                                                           \
    const auto *config = (gc9a01_config_t *)dev->config;                      \
    auto rc = pm_device_action_run(config->bus.bus, PM_DEVICE_ACTION_RESUME); \
    __ASSERT(rc == -EALREADY || rc == 0, "Failed resume SPI Bus");            \
  }

#define gc9a01_spi_suspend(dev)                                                \
  {                                                                            \
    const auto *config = (gc9a01_config_t *)dev->config;                       \
    auto rc = pm_device_action_run(config->bus.bus, PM_DEVICE_ACTION_SUSPEND); \
    __ASSERT(rc == -EALREADY || rc == 0, "Failed suspend SPI Bus");            \
  }

void gc9a01_clear(const device *dev, uint16_t color)
{
  gc9a01_set_frame(dev, 0, 0, DISPLAY_WIDTH - 1, DISPLAY_HEIGHT - 1);

  std::array<uint16_t, 100> rgb;
  rgb.fill(__bswap_16(color)); // spi writes this in the wrong endian-ness so we have to flip the byte order
  gc9a01_write_cmd(dev, GC9A01A_RAMWR);
  for (auto i = 0u; i < (DISPLAY_WIDTH * DISPLAY_HEIGHT) / rgb.size(); ++i)
  {
    gc9a01_write_data(dev, (uint8_t *)rgb.data(), rgb.size() * 2);
  }
  for (auto i = 0u; i < (DISPLAY_WIDTH * DISPLAY_HEIGHT) % rgb.size(); ++i)
  {
    gc9a01_write_data(dev, (uint8_t *)rgb.data(), ((DISPLAY_WIDTH * DISPLAY_HEIGHT) % rgb.size()) * 2);
  }
}

void gc9a01_clear(const device *dev, uint8_t r, uint8_t g, uint8_t b)
{
  gc9a01_clear(dev, rgb8_to_rgb565(r, g, b));
}

int gc9a01_write_cmd(const device *dev, uint8_t cmd)
{
  const auto *config = (gc9a01_config_t *)dev->config;
  struct spi_buf buf = {.buf = &cmd, .len = sizeof(cmd)};
  struct spi_buf_set buf_set = {.buffers = &buf, .count = 1};
  gpio_pin_set_dt(&config->dc_gpio, 0);
  if (spi_write_dt(&config->bus, &buf_set) != 0)
  {
    LOG_ERR("Failed sending command");
    return -EIO;
  }
  return 0;
}

int gc9a01_write_data(const device *dev, const uint8_t *data, size_t len)
{
  const auto *config = (gc9a01_config_t *)dev->config;
  struct spi_buf buf = {.buf = (void *)data, .len = len};
  struct spi_buf_set buf_set = {.buffers = &buf, .count = 1};
  gpio_pin_set_dt(&config->dc_gpio, 1);
  if (spi_write_dt(&config->bus, &buf_set) != 0)
  {
    LOG_ERR("Failed sending data");
    return -EIO;
  }
  return 0;
}

inline int gc9a01_write_cmd_data(const device *dev, uint8_t cmd, const uint8_t *data, size_t len)
{
  int err = gc9a01_write_cmd(dev, cmd);
  if (err)
  {
    return err;
  }
  if (data != NULL && len > 0)
  {
    return gc9a01_write_data(dev, data, len);
  }
  return 0;
}

void gc9a01_set_frame(const device *dev, const uint16_t x, const uint16_t y, const uint16_t endx, const uint16_t endy)
{
  uint8_t data[4];
  data[0] = (x >> 8) & 0xFF;
  data[1] = x & 0xFF;
  data[2] = (endx >> 8) & 0xFF;
  data[3] = endx & 0xFF;
  gc9a01_write_cmd_data(dev, COL_ADDR_SET, data, sizeof(data));

  data[0] = (y >> 8) & 0xFF;
  data[1] = y & 0xFF;
  data[2] = (endy >> 8) & 0xFF;
  data[3] = endy & 0xFF;
  gc9a01_write_cmd_data(dev, ROW_ADDR_SET, data, sizeof(data));
}

int gc9a01_init_display(const device *dev)
{
  const auto *config = (gc9a01_config_t *)dev->config;
  // reset diplsay
  gpio_pin_set_dt(&config->reset_gpio, 0);
  k_msleep(5);
  gpio_pin_set_dt(&config->reset_gpio, 1);
  k_msleep(150);

  gc9a01_spi_resume(dev);

  for (const auto &c : gc9a01_initcmds)
  {
    gc9a01_write_cmd_data(dev, c.cmd, c.argv, c.argc);
  }
  gc9a01_clear(dev, 0x0000);
  gc9a01_write_cmd(dev, GC9A01A_DISPON);
  k_msleep(150);
  gc9a01_write_cmd(dev, GC9A01A_SLPOUT);
  k_msleep(150);

  gc9a01_spi_suspend(dev);

  return 0;
}

int gc9a01_init(const device *dev)
{
  const auto *config = (gc9a01_config_t *)dev->config;
  if (!device_is_ready(config->reset_gpio.port))
  {
    LOG_ERR("Reset GPIO device not ready");
    return -ENODEV;
  }

  if (!device_is_ready(config->dc_gpio.port))
  {
    LOG_ERR("DC GPIO device not ready");
    return -ENODEV;
  }

  if (!device_is_ready(config->bl_gpio.port))
  {
    LOG_ERR("Busy GPIO device not ready");
    return -ENODEV;
  }

  gpio_pin_configure_dt(&config->reset_gpio, GPIO_OUTPUT_INACTIVE);
  gpio_pin_configure_dt(&config->dc_gpio, GPIO_OUTPUT_INACTIVE);
  gpio_pin_configure_dt(&config->bl_gpio, GPIO_OUTPUT_INACTIVE); // Default to 0 brightness

  return gc9a01_init_display(dev);
}

int gc9a01_blanking_on(const device *dev)
{
  return gc9a01_write_cmd(dev, GC9A01A_DISPOFF);
}

int gc9a01_blanking_off(const device *dev)
{
  return gc9a01_write_cmd(dev, GC9A01A_DISPON);
}

int gc9a01_write_buf(const device *dev,
                     const uint16_t x,
                     const uint16_t y,
                     const display_buffer_descriptor *desc,
                     const void *buf)
{
  gc9a01_spi_resume(dev);

  gc9a01_set_frame(dev, x, y, uint16_t(x + desc->width - 1), uint16_t(y + desc->height - 1));

  size_t len = desc->width * desc->height * 2; // TODO: look into desc->buf_size

  gc9a01_write_cmd_data(dev, GC9A01A_RAMWR, (uint8_t *)buf, len);

  gc9a01_spi_suspend(dev);
  return 0;
}

int gc9a01_read_buf(const struct device *dev,
                    const uint16_t x, const uint16_t y,
                    const struct display_buffer_descriptor *desc,
                    void *buf)
{
  return -ENOTSUP;
}

void *gc9a01_get_framebuffer(const struct device *dev)
{
  return NULL;
}

int gc9a01_set_brightness(const struct device *dev,
                          const uint8_t brightness)
{
  return -ENOTSUP;
}

int gc9a01_set_contrast(const struct device *dev, uint8_t contrast)
{
  return -ENOTSUP;
}
void gc9a01_get_capabilities(const struct device *dev,
                             struct display_capabilities *caps)
{
  memset(caps, 0, sizeof(struct display_capabilities));
  caps->x_resolution = DISPLAY_WIDTH;
  caps->y_resolution = DISPLAY_HEIGHT;
  caps->supported_pixel_formats = PIXEL_FORMAT_BGR_565;
  caps->current_pixel_format = PIXEL_FORMAT_BGR_565;
  caps->screen_info = SCREEN_INFO_MONO_MSB_FIRST;
}

static int gc9a01_set_orientation(const struct device *dev,
                                  const enum display_orientation
                                      orientation)
{
  gc9a01_spi_resume(dev);
  uint8_t data;
  switch (orientation)
  {
  case DISPLAY_ORIENTATION_NORMAL:
    data = ExchangeModeReverse | RowOrderBottomToTop | ColumnOrderRightToLeft | PixelOrderBGR;
    break;
  case DISPLAY_ORIENTATION_ROTATED_90:
    data = HorizonalRefreshLeftToRight | RowOrderBottomToTop | 0 | PixelOrderBGR;
    break;
  case DISPLAY_ORIENTATION_ROTATED_180:
    data = ExchangeModeReverse | 0 | 0 | PixelOrderBGR;
    break;
  case DISPLAY_ORIENTATION_ROTATED_270:
    data = HorizonalRefreshLeftToRight | ColumnOrderRightToLeft | 0 | 0 | PixelOrderBGR;
    break;
  }
  int err = gc9a01_write_cmd_data(dev, GC9A01A_MADCTL, &data, 1);
  gc9a01_spi_suspend(dev);
  return err;
}

static int gc9a01_set_pixel_format(const struct device *dev,
                                   const enum display_pixel_format pf)
{
  return -ENOTSUP;
}

int gc9a01_pm_action(const struct device *dev,
                     enum pm_device_action action)
{
  gc9a01_spi_resume(dev);

  auto err = 0;
  switch (action)
  {
  case PM_DEVICE_ACTION_RESUME:
    err = gc9a01_write_cmd(dev, GC9A01A_SLPOUT);
    k_msleep(5); // According to datasheet wait 5ms after SLPOUT before next command.
    err = gc9a01_write_cmd(dev, GC9A01A_DISPON);
    break;
  case PM_DEVICE_ACTION_SUSPEND:
    err = gc9a01_write_cmd(dev, GC9A01A_DISPOFF);
    err = gc9a01_write_cmd(dev, GC9A01A_SLPIN);
    break;
  case PM_DEVICE_ACTION_TURN_ON:
    err = gc9a01_init(dev);
    break;
  case PM_DEVICE_ACTION_TURN_OFF:
    break;
  default:
    err = -ENOTSUP;
  }

  gc9a01_spi_suspend(dev);

  if (err < 0)
  {
    LOG_ERR("%s: failed to set power mode", dev->name);
  }

  return err;
}

struct display_driver_api gc9a01_driver_api = {
    .blanking_on = gc9a01_blanking_on,
    .blanking_off = gc9a01_blanking_off,
    .write = gc9a01_write_buf,
    .read = gc9a01_read_buf,
    .get_framebuffer = gc9a01_get_framebuffer,
    .set_brightness = gc9a01_set_brightness,
    .set_contrast = gc9a01_set_contrast,
    .get_capabilities = gc9a01_get_capabilities,
    .set_pixel_format = gc9a01_set_pixel_format,
    .set_orientation = gc9a01_set_orientation,
};

const struct gc9a01_config_t gc9a01_configa = {
    .bus = SPI_DT_SPEC_INST_GET(0, SPI_OP_MODE_MASTER | SPI_WORD_SET(8), 0),
    .dc_gpio = GPIO_DT_SPEC_INST_GET(0, dc_gpios),
    .bl_gpio = GPIO_DT_SPEC_INST_GET(0, bl_gpios),
    .reset_gpio = GPIO_DT_SPEC_INST_GET(0, reset_gpios),
};

// fixes no user-provided default constructor
#undef Z_PM_DEVICE_DEFINE_SLOT
#define Z_PM_DEVICE_DEFINE_SLOT(dev_id)                                   \
  static const STRUCT_SECTION_ITERABLE_ALTERNATE(pm_device_slots, device, \
                                                 _CONCAT(__pm_slot_, dev_id)) {}

// fixes out of order initialization error
#undef Z_PM_DEVICE_INIT
#define Z_PM_DEVICE_INIT(obj, node_id, pm_action_cb)      \
  {                                                       \
    Z_PM_DEVICE_RUNTIME_INIT(obj)                         \
    Z_PM_DEVICE_POWER_DOMAIN_INIT(node_id)                \
        .flags = ATOMIC_INIT(Z_PM_DEVICE_FLAGS(node_id)), \
        .state = PM_DEVICE_STATE_ACTIVE,                  \
        .action_cb = pm_action_cb,                        \
  }

PM_DEVICE_DT_INST_DEFINE(0, gc9a01_pm_action);
DEVICE_DT_INST_DEFINE(0,
                      gc9a01_init,
                      PM_DEVICE_DT_INST_GET(0),
                      NULL,
                      &gc9a01_configa,
                      POST_KERNEL,
                      CONFIG_DISPLAY_INIT_PRIORITY,
                      &gc9a01_driver_api);
