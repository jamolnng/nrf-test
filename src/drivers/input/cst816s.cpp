#define DT_DRV_COMPAT hynitron_cst816s

#include <zephyr/sys/byteorder.h>
#include <zephyr/input/input.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>

constexpr uint8_t CST816S_CHIP_ID = 0xB4u;

enum class Register : uint8_t
{
  Data = 0x00,
  GestureID = 0x01,
  FingerNum = 0x02,
  XPosH = 0x03,
  XPosL = 0x04,
  YPosH = 0x05,
  YPosL = 0x06,
  BPC0H = 0xB0,
  BPC0L = 0xB1,
  BPC1H = 0xB2,
  BPC1L = 0xB3,
  PowerMode = 0xA5,
  ChipID = 0xA7,
  ProjID = 0xA8,
  FWVersion = 0xA9,
  MotionMask = 0xEC,
  IRQPulseWidth = 0xED,
  NORScanPer = 0xEE,
  MotionS1Angle = 0xEF,
  LPScanRaw1H = 0xF0,
  LPScanRaw1L = 0xF1,
  LPScanRaw2H = 0xF2,
  LPScanRaw2L = 0xF3,
  LPAutoWakeupTime = 0xF4,
  LPScanTH = 0xF5,
  LPScanWIN = 0xF6,
  LPScanFreq = 0xF7,
  LPScanIDAC = 0xF8,
  AutoSleepTime = 0xF9,
  IRQControl = 0xFA,
  DebounceTime = 0xFB,
  LongPressTime = 0xFC,
  IOControl = 0xFD,
  DISAutoSleep = 0xFE,
};

enum class Motion : uint8_t
{
  CON_LR = BIT(2),
  CON_UD = BIT(1),
  DClick = BIT(0),
};

enum class IRQ : uint8_t
{
  EnableTest = BIT(7),
  EnableTouch = BIT(6),
  EnableChange = BIT(5),
  EnableMotion = BIT(4),
  Once_WLP = BIT(0),
};

enum class IOCTL : uint8_t
{
  SoftRTS = BIT(2),
  IIC_OD = BIT(1),
  EN_1V8 = BIT(0),
};

enum class PowerMode : uint8_t
{
  Sleep = 0x03,
  Experimental = 0x05,
};

enum class Gesture : uint8_t
{
  None = 0x00,
  UpSliding = 0x01,
  DownSliding = 0x02,
  LeftSlide = 0x03,
  RightSlide = 0x04,
  Click = 0x05,
  DoubleClick = 0x0B,
  LongPress = 0x0C,
};

enum class Event : uint8_t
{
  PressDown = 0x00,
  LiftUp = 0x01,
  Contact = 0x02,
  None = 0x03,
};

constexpr uint8_t CST816S_EVENT_BITS_POS = 0x06;

constexpr uint8_t CST816S_RESET_DELAY_MS = 5; /* in ms */
constexpr uint8_t CST816S_WAIT_DELAY_MS = 50; /* in ms */

struct cst816s_config
{
  struct i2c_dt_spec i2c;
  const gpio_dt_spec rst_gpio;

#ifdef CONFIG_INPUT_CST816S_INTERRUPT
  const gpio_dt_spec int_gpio;
#endif
};

struct cst816s_data
{
  const device *dev;
  struct k_work work;

#ifdef CONFIG_INPUT_CST816S_INTERRUPT
  gpio_callback int_gpio_cb;
#else
  struct k_timer timer;
#endif
};

struct cst816s_output
{
  Gesture gesture;
  uint8_t points;
  uint16_t x;
  uint16_t y;
};

LOG_MODULE_REGISTER(cst816s, CONFIG_INPUT_LOG_LEVEL);

static int cst816s_process(const device *dev)
{
  const auto *config = (cst816s_config *)dev->config;

  cst816s_output output;
  if (i2c_burst_read_dt(&config->i2c, (uint8_t)Register::GestureID, (uint8_t *)&output, sizeof(cst816s_output)) < 0)
  {
    LOG_ERR("Could not read data");
    return -ENODATA;
  }

  uint16_t x = sys_be16_to_cpu(output.x) & 0x0FFF;
  uint16_t y = sys_be16_to_cpu(output.y) & 0x0FFF;
  uint8_t event = (output.x & 0xFF) >> CST816S_EVENT_BITS_POS;

  bool pressed = event == (uint8_t)Event::Contact;

  LOG_DBG("x: %u, y: %u, npoints: %u, gesture: %u, event: %u, pressed: %u",
          x,
          y,
          output.points,
          (uint8_t)output.gesture,
          event,
          pressed);

  if (pressed)
  {
    input_report_abs(dev, INPUT_ABS_X, x, false, K_FOREVER);
    input_report_abs(dev, INPUT_ABS_Y, y, false, K_FOREVER);
    input_report_key(dev, INPUT_BTN_TOUCH, 1, true, K_FOREVER);
  }
  else
  {
    input_report_key(dev, INPUT_BTN_TOUCH, 0, true, K_FOREVER);
    switch (output.gesture)
    {
    case Gesture::UpSliding:
      input_report_key(dev, INPUT_BTN_NORTH, 0, true, K_FOREVER);
      break;
    case Gesture::DownSliding:
      input_report_key(dev, INPUT_BTN_SOUTH, 0, true, K_FOREVER);
      break;
    case Gesture::LeftSlide:
      input_report_key(dev, INPUT_BTN_WEST, 0, true, K_FOREVER);
      break;
    case Gesture::RightSlide:
      input_report_key(dev, INPUT_BTN_EAST, 0, true, K_FOREVER);
      break;
    case Gesture::Click:
      // input_report_key(dev, INPUT_BTN_MIDDLE, 0, true, K_FOREVER);
      break;
    case Gesture::DoubleClick:
      // input_report_key(dev, INPUT_BTN_MIDDLE, 0, true, K_FOREVER);
      break;
    case Gesture::LongPress:
      // input_report_key(dev, INPUT_BTN_MIDDLE, 0, true, K_FOREVER);
      break;
    }
  }

  return 0;
}

static void cst816s_work_handler(k_work *work)
{
  struct cst816s_data *data = CONTAINER_OF(work, struct cst816s_data, work);

  cst816s_process(data->dev);
}

#ifdef CONFIG_INPUT_CST816S_INTERRUPT
static void cst816s_isr_handler(const device *dev, gpio_callback *cb, uint32_t mask)
{
  cst816s_data *data = CONTAINER_OF(cb, cst816s_data, int_gpio_cb);

  k_work_submit(&data->work);
}
#else
static void cst816s_timer_handler(k_timer *timer)
{
  cst816s_data *data = CONTAINER_OF(timer, cst816s_data, timer);

  k_work_submit(&data->work);
}
#endif

static void cst816s_chip_reset(const device *dev)
{
  const auto *config = (cst816s_config *)dev->config;

  if (gpio_is_ready_dt(&config->rst_gpio))
  {
    if (gpio_pin_configure_dt(&config->rst_gpio, GPIO_OUTPUT_INACTIVE) < 0)
    {
      LOG_ERR("Could not configure reset GPIO pin");
      return;
    }

    gpio_pin_set_dt(&config->rst_gpio, 1);
    k_msleep(CST816S_RESET_DELAY_MS);
    gpio_pin_set_dt(&config->rst_gpio, 0);
    k_msleep(CST816S_WAIT_DELAY_MS);
  }
}

static int cst816s_chip_init(const device *dev)
{
  const auto *config = (cst816s_config *)dev->config;
  uint8_t chip_id;

  cst816s_chip_reset(dev);

  if (!device_is_ready(config->i2c.bus))
  {
    LOG_ERR("I2C bus %s not ready", config->i2c.bus->name);
    return -ENODEV;
  }

  if (i2c_reg_read_byte_dt(&config->i2c, (uint8_t)Register::ChipID, &chip_id) < 0)
  {
    LOG_ERR("failed reading chip id");
    return -ENODATA;
  }

  if (chip_id != CST816S_CHIP_ID)
  {
    LOG_ERR("CST816S wrong chip id: returned 0x%x", chip_id);
    return -ENODEV;
  }

  if (i2c_reg_update_byte_dt(&config->i2c, (uint8_t)Register::MotionMask,
                             (uint8_t)Motion::DClick | (uint8_t)Motion::CON_LR | (uint8_t)Motion::CON_UD,
                             (uint8_t)Motion::DClick | (uint8_t)Motion::CON_LR | (uint8_t)Motion::CON_UD) < 0)
  {
    LOG_ERR("Could not set motion mask");
    return -ENODATA;
  }

  if (i2c_reg_update_byte_dt(&config->i2c, (uint8_t)Register::IRQControl,
                             (uint8_t)IRQ::EnableMotion | (uint8_t)IRQ::EnableTouch | (uint8_t)IRQ::EnableChange,
                             (uint8_t)IRQ::EnableMotion | (uint8_t)IRQ::EnableTouch | (uint8_t)IRQ::EnableChange) < 0)
  {
    LOG_ERR("Could not enable irq");
    return -ENODATA;
  }

  return 0;
}

static int cst816s_init(const device *dev)
{
  auto *data = (cst816s_data *)dev->data;

  data->dev = dev;
  k_work_init(&data->work, cst816s_work_handler);

  LOG_DBG("Initialize CST816S");

#ifdef CONFIG_INPUT_CST816S_INTERRUPT
  const auto *config = (cst816s_config *)dev->config;

  if (!gpio_is_ready_dt(&config->int_gpio))
  {
    LOG_ERR("GPIO port %s not ready", config->int_gpio.port->name);
    return -EIO;
  }

  if (gpio_pin_configure_dt(&config->int_gpio, GPIO_INPUT) < 0)
  {
    LOG_ERR("Could not configure interrupt GPIO pin");
    return -EIO;
  }

  if (gpio_pin_interrupt_configure_dt(&config->int_gpio, GPIO_INT_EDGE_TO_ACTIVE) < 0)
  {
    LOG_ERR("Could not configure interrupt GPIO interrupt.");
    return -EIO;
  }

  gpio_init_callback(&data->int_gpio_cb, cst816s_isr_handler, BIT(config->int_gpio.pin));

  if (gpio_add_callback(config->int_gpio.port, &data->int_gpio_cb) < 0)
  {
    LOG_ERR("Could not set gpio callback");
    return -EIO;
  }
#else
  k_timer_init(&data->timer, cst816s_timer_handler, NULL);
  k_timer_start(&data->timer, K_MSEC(CONFIG_INPUT_CST816S_PERIOD), K_MSEC(CONFIG_INPUT_CST816S_PERIOD));
#endif

  return cst816s_chip_init(dev);
};

#ifdef CONFIG_PM_DEVICE
static int cst816s_pm_action(const device *dev, enum pm_device_action action)
{
  const auto *config = (cst816s_config *)dev->config;
  int status;

  LOG_DBG("Status: %u", action);

  switch (action)
  {
  case PM_DEVICE_ACTION_SUSPEND:
  {
    LOG_DBG("State changed to suspended");
    if (device_is_ready(config->rst_gpio.port))
    {
      status = gpio_pin_set_dt(&config->rst_gpio, 1);
    }

    break;
  }
  case PM_DEVICE_ACTION_RESUME:
  {
    LOG_DBG("State changed to active");
    status = cst816s_chip_init(dev);

    break;
  }
  default:
  {
    return -ENOTSUP;
  }
  }

  return status;
}
#endif

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

#define CST816S_DEFINE(index)                                                             \
  static struct cst816s_data cst816s_data_##index;                                        \
  static const struct cst816s_config cst816s_config_##index = {                           \
      .i2c = I2C_DT_SPEC_INST_GET(index),                                                 \
      .rst_gpio = GPIO_DT_SPEC_INST_GET_OR(index, rst_gpios, {}),                         \
      COND_CODE_1(CONFIG_INPUT_CST816S_INTERRUPT,                                         \
                  (.int_gpio = GPIO_DT_SPEC_INST_GET(index, irq_gpios), ), ())};          \
                                                                                          \
  PM_DEVICE_DT_INST_DEFINE(index, cst816s_pm_action);                                     \
                                                                                          \
  DEVICE_DT_INST_DEFINE(index, cst816s_init, NULL, &cst816s_data_##index,                 \
                        &cst816s_config_##index, POST_KERNEL, CONFIG_INPUT_INIT_PRIORITY, \
                        NULL);

DT_INST_FOREACH_STATUS_OKAY(CST816S_DEFINE)