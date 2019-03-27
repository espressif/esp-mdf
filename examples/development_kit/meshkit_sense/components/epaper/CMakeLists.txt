
set(COMPONENT_SRCS "epaper_font.c"
                    "epaper.c")

set(COMPONENT_ADD_INCLUDEDIRS "."
                                "include")

# requirements can't depend on config
set(COMPONENT_REQUIRES mcommon mwifi mlink i2c_bus)

register_component()
