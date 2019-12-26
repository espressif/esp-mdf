
set(COMPONENT_SRCS "app_peripheral.cpp"
                    "buddy_cmd.c"
                    "buddy_example.c"
                    "buddy_recv.cpp")

set(COMPONENT_ADD_INCLUDEDIRS ". include")

# requirements can't depend on config
set(COMPONENT_REQUIRES mcommon mwifi mlink mdebug attenuator button hts221 i2c_bus ssd1306 ws2812b mesh_utils)

register_component()
