
set(COMPONENT_SRCS "mcommon_examples.c")

set(COMPONENT_ADD_INCLUDEDIRS ".")

# requirements can't depend on config
set(COMPONENT_REQUIRES mcommon mwifi mlink)

register_component()
