idf_component_register(SRCS "./app_peripheral.cpp" "./buddy_cmd.c" "./buddy_example.c" "./buddy_recv.cpp"
                    INCLUDE_DIRS "." "./include"
                    REQUIRES mcommon mwifi mlink mdebug attenuator button hts221 i2c_bus ssd1306 ws2812b mesh_utils
)