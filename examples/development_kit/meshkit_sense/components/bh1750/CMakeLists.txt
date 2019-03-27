
set(COMPONENT_SRCS "bh1750.c"
                    "bh1750_obj.cpp")

set(COMPONENT_ADD_INCLUDEDIRS ". include")

# requirements can't depend on config
set(COMPONENT_REQUIRES mcommon mwifi mlink i2c_bus)

register_component()
