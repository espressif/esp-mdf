
set(COMPONENT_SRCS "mlink_espnow.c"
                    "mlink_handle.c"
                    "mlink_httpd.c"
                    "mlink_json.c"
                    "mlink_notice.c"
                    "mlink_trigger.c"
                    "mlink_utils.c")

set(COMPONENT_ADD_INCLUDEDIRS "include")

# requirements can't depend on config
set(COMPONENT_REQUIRES mcommon mespnow mwifi mupgrade mconfig json mdns esp_http_server)

register_component()
