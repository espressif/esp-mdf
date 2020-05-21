
set(COMPONENT_SRCS "mdebug_cmd.c"
                   "mdebug_console.c"
                   "mdebug_espnow.c"
                   "mdebug_flash.c"
                   "mdebug_log.c")

set(COMPONENT_INCLUDEDIRS "include")

idf_component_register(SRCS "${COMPONENT_SRCS}"
                    INCLUDE_DIRS "${COMPONENT_INCLUDEDIRS}"
                    REQUIRES mcommon mespnow mwifi mupgrade mconfig json mdns esp_http_server vfs fatfs wpa_supplicant)
