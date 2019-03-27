
set(COMPONENT_SRCS "sense_epaper.cpp"
                    "sense_led.cpp"
                    "sense_main.c"
                    "sense_sensors.cpp")

set(COMPONENT_ADD_INCLUDEDIRS "include")

# requirements can't depend on config
set(COMPONENT_REQUIRES mcommon mwifi mlink epaper hts221 bh1750 ulp_operation base_class led button)

register_component()
