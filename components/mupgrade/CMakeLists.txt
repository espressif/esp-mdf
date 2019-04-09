
set(COMPONENT_SRCS "mupgrade_check.c"
                    "mupgrade_node.c"
                    "mupgrade_root.c")

set(COMPONENT_ADD_INCLUDEDIRS "include")

# requirements can't depend on config
set(COMPONENT_REQUIRES mcommon mespnow mwifi json mdns esp_http_server app_update)

register_component()
