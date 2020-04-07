#ifndef __ETH_HELPER_H
#define __ETH_HELPER_H

#include "esp_err.h"
#include "esp_eth.h"
#include "esp_event_loop.h"
#include "esp_event.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "tcpip_adapter.h"
#include "sdkconfig.h"

#if CONFIG_PHY_LAN8720
#include "eth_phy/phy_lan8720.h"
#define DEFAULT_ETHERNET_PHY_CONFIG phy_lan8720_default_ethernet_config
#elif CONFIG_PHY_TLK110
#include "eth_phy/phy_tlk110.h"
#define DEFAULT_ETHERNET_PHY_CONFIG phy_tlk110_default_ethernet_config
#elif CONFIG_PHY_IP101
#include "eth_phy/phy_ip101.h"
#define DEFAULT_ETHERNET_PHY_CONFIG phy_ip101_default_ethernet_config
#endif

typedef struct {
    void (*eth_connect_event)(void);    /*!< eth connect event callback */
    void (*eth_disconnect_event)(void); /*!< eth disconnect event callback */
    void (*eth_start_event)(void);      /*!< eth start event callback */
    void (*eth_stop_event)(void);       /*!< eth stop callback */
    void (*got_ip)(void);               /*!< got ip event callback */
} eth_helper_init_t;                    /*!< eth helper */

/**
  * @brief Use Eth_helper to initialize Ethernet
  *
  * @param eth_helper_init_struct Initialize param
  *
  * @return
  *	    - ESP_OK if sucess
  *	    - ESP_ERR_INVALID_ARG if param is error
  *	    - ESP_ERR_INVALID_STATE if call this function twice or more
  *	    - ESP_ERR_NO_MEM if memory is not enough
*/
esp_err_t eth_helper_init(eth_helper_init_t *eth_helper_init_struct);

/**
  * @brief Use Eth_helper to deinitialize Ethernet
  *
  * @return
  *	    - ESP_OK if sucess
  *	    - ESP_ERR_INVALID_STATE if ethernet has not been initialize
*/
esp_err_t eth_helper_deinit();

#endif /* end of include guard: __ETH_HELPER_H */
