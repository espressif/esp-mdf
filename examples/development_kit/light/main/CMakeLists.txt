
set(COMPONENT_SRCS "light_example.c")

set(COMPONENT_ADD_INCLUDEDIRS ".")

# requirements can't depend on config
set(COMPONENT_REQUIRES mcommon mwifi mlink light_driver light_handle mesh_utils)

register_component()
