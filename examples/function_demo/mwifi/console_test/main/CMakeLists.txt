
set(COMPONENT_SRCS "./mwifi_test.c")

set(COMPONENT_ADD_INCLUDEDIRS "")

# requirements can't depend on config
set(COMPONENT_REQUIRES mcommon mespnow mwifi mdebug)

register_component()
