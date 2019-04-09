
set(COMPONENT_SRCS "button.c")

set(COMPONENT_ADD_INCLUDEDIRS ".")

# requirements can't depend on config
set(COMPONENT_REQUIRES mwifi mlink button_driver)

register_component()