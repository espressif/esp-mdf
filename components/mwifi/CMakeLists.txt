
set(COMPONENT_SRCS "mwifi.c")

set(COMPONENT_ADD_INCLUDEDIRS "include")

# requirements can't depend on config
set(COMPONENT_REQUIRES mcommon miniz)

register_component()
