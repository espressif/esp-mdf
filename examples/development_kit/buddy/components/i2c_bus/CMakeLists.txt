
set(COMPONENT_SRCS "i2c_bus.c"
                    "i2c_bus_obj.cpp")

set(COMPONENT_ADD_INCLUDEDIRS "." "include")
set(COMPONENT_REQUIRES spi_flash partition_table)

register_component()
