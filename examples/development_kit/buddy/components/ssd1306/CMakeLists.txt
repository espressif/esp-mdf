
set(COMPONENT_SRCS "ssd1306_fonts.c"
                    "ssd1306_obj.cpp"
                    "ssd1306.c")

set(COMPONENT_ADD_INCLUDEDIRS "." "include")

set(COMPONENT_REQUIRES i2c_bus)

register_component()
