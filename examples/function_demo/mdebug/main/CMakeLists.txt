
set(COMPONENT_SRCS "mdebug_example.c")

set(COMPONENT_ADD_INCLUDEDIRS ".")

# requirements can't depend on config
set(COMPONENT_REQUIRES mcommon mwifi mlink mdebug)

register_component()
