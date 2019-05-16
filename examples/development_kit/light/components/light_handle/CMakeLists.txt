
set(COMPONENT_SRCS "light_handle.c")

set(COMPONENT_ADD_INCLUDEDIRS ". include")

# requirements can't depend on config
set(COMPONENT_REQUIRES mcommon mlink light_driver)

register_component()
