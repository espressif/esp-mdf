
set(COMPONENT_SRCS "button_driver.c"
                    "iot_button.c"
                    "iot_light.c")

set(COMPONENT_ADD_INCLUDEDIRS ". include")

# requirements can't depend on config
set(COMPONENT_REQUIRES mcommon esp_adc_cal)

register_component()
