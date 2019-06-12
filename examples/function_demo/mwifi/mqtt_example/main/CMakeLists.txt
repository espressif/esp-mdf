
set(COMPONENT_SRCS "mqtt_example.c")

set(COMPONENT_ADD_INCLUDEDIRS "")

# requirements can't depend on config
set(COMPONENT_REQUIRES mcommon mconfig mwifi mlink mesh_mqtt_handle)

register_component()
