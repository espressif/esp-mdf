
set(COMPONENT_SRCS "i2c_bus.c"
                    "i2c_bus_obj.cpp")

set(COMPONENT_ADD_INCLUDEDIRS ". include")

# requirements can't depend on config
set(COMPONENT_REQUIRES mcommon mwifi mlink)

register_component()
