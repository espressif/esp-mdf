#include "eth_helper.h"
#include "string.h"

#define PIN_PHY_POWER CONFIG_PHY_POWER_PIN
#define PIN_SMI_MDC CONFIG_PHY_SMI_MDC_PIN
#define PIN_SMI_MDIO CONFIG_PHY_SMI_MDIO_PIN

#define ETH_HELPER_CHECK(expect,str,goto_tag,ret_value,...) \
    do{ \
        if(!(expect)){ \
            ESP_LOGE(TAG,"%s(%d): " str,__func__,__LINE__,##__VA_ARGS__); \
            ret =  ret_value; \
            goto goto_tag; \
        } \
    }while(0);
#define ETH_EXECUTE_CALLBACK(user_cb,cb_1,...) \
    if((user_cb)->cb_1) \
        (user_cb)->cb_1(__VA_ARGS__); \

static const char *TAG = "eth_helper";

typedef struct {
    void (*eth_connect_event)(void);
    void (*eth_disconnect_event)(void);
    void (*eth_start_event)(void);
    void (*eth_stop_event)(void);
    void (*got_ip_event)(void);
} eth_helper_t;
static eth_helper_t *g_eth_helper;

#ifdef CONFIG_PHY_USE_POWER_PIN
/**
 * @brief re-define power enable func for phy
 *
 * @param enable true to enable, false to disable
 *
 * @note This function replaces the default PHY power on/off function.
 * If this GPIO is not connected on your device (and PHY is always powered),
 * you can use the default PHY-specific power on/off function.
 */
static void phy_device_power_enable_via_gpio(bool enable)
{
    assert(DEFAULT_ETHERNET_PHY_CONFIG.phy_power_enable);

    if (!enable) {
        DEFAULT_ETHERNET_PHY_CONFIG.phy_power_enable(false);
    }

    gpio_pad_select_gpio(PIN_PHY_POWER);
    gpio_set_direction(PIN_PHY_POWER, GPIO_MODE_OUTPUT);

    if (enable == true) {
        gpio_set_level(PIN_PHY_POWER, 1);
        ESP_LOGI(TAG, "Power On Ethernet PHY");
    } else {
        gpio_set_level(PIN_PHY_POWER, 0);
        ESP_LOGI(TAG, "Power Off Ethernet PHY");
    }

    vTaskDelay(1); // Allow the power up/down to take effect, min 300us

    if (enable) {
        /* call the default PHY-specific power on function */
        DEFAULT_ETHERNET_PHY_CONFIG.phy_power_enable(true);
    }
}
#endif

/**
 * @brief gpio specific init
 *
 * @note RMII data pins are fixed in esp32:
 * TXD0 <=> GPIO19
 * TXD1 <=> GPIO22
 * TX_EN <=> GPIO21
 * RXD0 <=> GPIO25
 * RXD1 <=> GPIO26
 * CLK <=> GPIO0
 *
 */
static void eth_gpio_config_rmii(void)
{
    phy_rmii_configure_data_interface_pins();
    phy_rmii_smi_configure_pins(PIN_SMI_MDC, PIN_SMI_MDIO);
}

/** Event handler for Ethernet events */
static esp_err_t eth_event_handler(void *ctx, system_event_t *event)
{
    tcpip_adapter_ip_info_t ip;

    switch (event->event_id) {
        case SYSTEM_EVENT_ETH_CONNECTED:
            ESP_LOGI(TAG, "Ethernet Link Up");
            ETH_EXECUTE_CALLBACK(g_eth_helper, eth_connect_event);
            break;

        case SYSTEM_EVENT_ETH_DISCONNECTED:
            ESP_LOGI(TAG, "Ethernet Link Down");
            ETH_EXECUTE_CALLBACK(g_eth_helper, eth_disconnect_event);
            break;

        case SYSTEM_EVENT_ETH_START:
            ESP_LOGI(TAG, "Ethernet Started");
            ETH_EXECUTE_CALLBACK(g_eth_helper, eth_start_event);
            break;

        case SYSTEM_EVENT_ETH_STOP:
            ESP_LOGI(TAG, "Ethernet Stopped");
            ETH_EXECUTE_CALLBACK(g_eth_helper, eth_stop_event);
            free(g_eth_helper);
            g_eth_helper = NULL;
            break;

        case SYSTEM_EVENT_ETH_GOT_IP:
            memset(&ip, 0, sizeof(tcpip_adapter_ip_info_t));
            ESP_ERROR_CHECK(tcpip_adapter_get_ip_info(ESP_IF_ETH, &ip));
            ESP_LOGI(TAG, "Ethernet Got IP Address");
            ESP_LOGI(TAG, "~~~~~~~~~~~");
            ESP_LOGI(TAG, "ETHIP:" IPSTR, IP2STR(&ip.ip));
            ESP_LOGI(TAG, "ETHMASK:" IPSTR, IP2STR(&ip.netmask));
            ESP_LOGI(TAG, "ETHGW:" IPSTR, IP2STR(&ip.gw));
            ESP_LOGI(TAG, "~~~~~~~~~~~");
            ETH_EXECUTE_CALLBACK(g_eth_helper, got_ip_event);
            break;

        default:
            break;
    }

    return ESP_OK;
}

esp_err_t eth_helper_init(eth_helper_init_t *eth_helper_init_struct)
{
    esp_err_t ret = ESP_OK;
    ETH_HELPER_CHECK(eth_helper_init_struct != NULL, "eth_helper_init_struct should not be NULL", direct_ret, ESP_ERR_INVALID_ARG)
    ETH_HELPER_CHECK(g_eth_helper == NULL, "g_eth_helper should only be initilized once", direct_ret, ESP_ERR_INVALID_STATE);
    ETH_HELPER_CHECK((g_eth_helper = (eth_helper_t *)calloc(1, sizeof(eth_helper_t))) != NULL, "memory is not enough", direct_ret, ESP_ERR_NO_MEM);
    g_eth_helper->got_ip_event = eth_helper_init_struct->got_ip;
    g_eth_helper->eth_connect_event = eth_helper_init_struct->eth_connect_event;
    g_eth_helper->eth_disconnect_event = eth_helper_init_struct->eth_disconnect_event;
    g_eth_helper->eth_start_event = eth_helper_init_struct->eth_start_event;
    g_eth_helper->eth_stop_event = eth_helper_init_struct->eth_stop_event;

    tcpip_adapter_init();

    ETH_HELPER_CHECK((ret = esp_event_loop_init(eth_event_handler, NULL)) == ESP_OK, " %s", free_before_ret, ret, esp_err_to_name(ret));

    eth_config_t config = DEFAULT_ETHERNET_PHY_CONFIG;
    config.phy_addr = CONFIG_PHY_ADDRESS;
    config.gpio_config = eth_gpio_config_rmii;
    config.tcpip_input = tcpip_adapter_eth_input;
    config.clock_mode = CONFIG_PHY_CLOCK_MODE;
#ifdef CONFIG_PHY_USE_POWER_PIN
    /* Replace the default 'power enable' function with an example-specific one
     that toggles a power GPIO. */
    config.phy_power_enable = phy_device_power_enable_via_gpio;
#endif

    ETH_HELPER_CHECK((ret = esp_eth_init(&config)) == ESP_OK, " %s", free_before_ret, ret, esp_err_to_name(ret));
    ETH_HELPER_CHECK((ret = esp_eth_enable()) == ESP_OK, " %s", free_before_ret, ret, esp_err_to_name(ret));

direct_ret:
    return ret;

free_before_ret:
    free(g_eth_helper);
    g_eth_helper = NULL;
    return ret;
}

esp_err_t eth_helper_deinit()
{
    esp_err_t ret = ESP_OK;
    ETH_HELPER_CHECK(g_eth_helper != NULL, "eth_helper has not been initialized", direct_ret, ESP_ERR_INVALID_STATE);
    ETH_HELPER_CHECK((ret = esp_eth_deinit()) == ESP_OK, " %s", direct_ret, ret, esp_err_to_name(ret));
direct_ret:
    return ret;
}
