
set(COMPONENT_SRCS "mlink_espnow.c"
                    "mlink_handle.c"
                    "mlink_httpd.c"
                    "mlink_json.c"
                    "mlink_notice.c"
                    "mlink_trigger.c"
                    "mlink_utils.c"
                    "mlink_ble.c"
                    "mlink_sniffer.c")

set(COMPONENT_INCLUDEDIRS "include")

# requirements can't depend on config
set(COMPONENT_REQUIRES mcommon mespnow mwifi mupgrade mconfig json mdns esp_http_server)

idf_component_register(SRCS "${COMPONENT_SRCS}"
                    INCLUDE_DIRS "${COMPONENT_INCLUDEDIRS}"
                    REQUIRES mcommon mespnow mwifi mupgrade mconfig json mdns esp_http_server)
