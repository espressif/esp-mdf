
set(COMPONENT_SRCS "mespnow.c")

set(COMPONENT_ADD_INCLUDEDIRS "include")

# requirements can't depend on config
set(COMPONENT_REQUIRES mcommon)

register_component()
