
set(COMPONENT_SRCS "hts221.c"
                    "hts221_obj.cpp")

set(COMPONENT_ADD_INCLUDEDIRS "." "include")

# requirements can't depend on config
set(COMPONENT_REQUIRES mcommon mwifi mconfig i2c_bus)

register_component()
