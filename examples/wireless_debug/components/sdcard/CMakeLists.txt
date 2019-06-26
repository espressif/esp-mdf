
set(COMPONENT_SRCS "sdcard.c")

set(COMPONENT_ADD_INCLUDEDIRS ". include")

# requirements can't depend on config
set(COMPONENT_REQUIRES mcommon mwifi mdebug)

register_component()
