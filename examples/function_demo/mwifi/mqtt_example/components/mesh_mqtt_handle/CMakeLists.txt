
set(COMPONENT_SRCS "mesh_mqtt_handle.c")

set(COMPONENT_ADD_INCLUDEDIRS "include")

# requirements can't depend on config
set(COMPONENT_REQUIRES mcommon mconfig mwifi mlink mqtt)

register_component()
