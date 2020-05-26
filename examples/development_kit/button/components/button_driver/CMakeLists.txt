
idf_component_register(SRCS "./button_driver.c" "./iot_button.c" "./iot_light"
                    INCLUDE_DIRS "." "./include"
                    REQUIRES mcommon esp_adc_cal
)
