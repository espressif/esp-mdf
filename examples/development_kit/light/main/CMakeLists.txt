
set(COMPONENT_SRCS "light.c")

set(COMPONENT_ADD_INCLUDEDIRS ".")

# requirements can't depend on config
set(COMPONENT_REQUIRES mcommon mwifi mlink light_driver)

register_component()
