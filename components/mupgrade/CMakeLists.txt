
set(COMPONENT_SRCS "mupgrade_check.c"
                    "mupgrade_node.c"
                    "mupgrade_root.c")

set(COMPONENT_INCLUDEDIRS "include")

idf_component_register(SRCS "${COMPONENT_SRCS}"
                    INCLUDE_DIRS "${COMPONENT_INCLUDEDIRS}"
                    REQUIRES mcommon mespnow mwifi json mdns esp_http_server app_update)

