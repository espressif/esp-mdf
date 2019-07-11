
set(COMPONENT_SRCS "controllable_obj.cpp")

set(COMPONENT_ADD_INCLUDEDIRS ". include")

# requirements can't depend on config
set(COMPONENT_REQUIRES mcommon mwifi mlink)

register_component()
