#pragma once

#include <zephyr/bluetooth/conn.h>

namespace bt::gatt_dm
{
      enum Service
      {
#ifdef CONFIG_BT_GATTP
            GAP_Client,
#endif
#ifdef CONFIG_GATT_0X1801_CLIENT
            GATT_0X1801_Client,
#endif
#ifdef CONFIG_BT_CTS_CLIENT
            CTS_Client,
#endif
#ifdef CONFIG_BT_AMS_CLIENT
            AMS_Client,
#endif
#ifdef CONFIG_BT_ANCS_CLIENT
            ANCS_Client,
#endif
      };
      void start(bt_conn *conn);
      bool has(Service service);
}