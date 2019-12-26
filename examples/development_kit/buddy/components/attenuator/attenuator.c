#include "driver/spi_master.h"
#include "esp_log.h"
#include "attenuator.h"

#define ATTENUATOR_PIN_LE       22
#define ATTENUATOR_PIN_CLK      21
#define ATTENUATOR_PIN_SI       16

static spi_device_handle_t spi;
static char *TAG = "attenuator";

void attenuator_set(uint8_t att_val)
{
    if (att_val > MAX_ATT_SUPPORTED) {
        ESP_LOGE(TAG, "attenuation data out of range: %d", att_val);
        return;
    }

    spi_transaction_t t = {0};
    uint8_t data[2] = {0};

    att_val = att_val << 2;

    // do bit reverse
    for (uint8_t i = 1; i < 8; i++) {
        data[0] += att_val & 0x01;
        att_val >>= 1;
        data[0] <<= 1;
    }

    ESP_LOGD(TAG, "data %x, %x", data[0], data[1]);
    t.length = 16;
    t.tx_buffer = &data;
    t.user = NULL;

    ESP_ERROR_CHECK(spi_device_polling_transmit(spi, &t));    //Transmit!
}

void attenuator_init(void)
{
    spi_bus_config_t buscfg = {
        .miso_io_num = -1,
        .mosi_io_num = ATTENUATOR_PIN_SI,
        .sclk_io_num = ATTENUATOR_PIN_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 16,
        .flags = SPICOMMON_BUSFLAG_MASTER | SPICOMMON_BUSFLAG_SCLK | SPICOMMON_BUSFLAG_MOSI,
    };

    spi_device_interface_config_t devcfg = {
        .command_bits = 0,
        .address_bits = 0,
        .clock_speed_hz = 1 * 1000 * 1000,          //Clock out at 1 MHz
        .mode = 0,                                  //SPI mode 0
        .spics_io_num = ATTENUATOR_PIN_LE,          //CS pin
        .queue_size = 4,                            //We want to be able to queue 7 transactions at a time
    };

    ESP_ERROR_CHECK(spi_bus_initialize(HSPI_HOST, &buscfg, 1));
    ESP_ERROR_CHECK(spi_bus_add_device(HSPI_HOST, &devcfg, &spi));

    // set to default value
    attenuator_set(DEFAULT_ATT_POWER_ON);
    ESP_LOGD(TAG, "att init");
}
