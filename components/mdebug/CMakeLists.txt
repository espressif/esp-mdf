
set(COMPONENT_SRCS "mdebug_cmd.c"
                   "mdebug_console.c"
                   "mdebug_espnow.c"
                   "mdebug_flash.c"
                   "mdebug_log.c")
set(COMPONENT_ADD_INCLUDEDIRS "include")

# requirements can't depend on config
set(COMPONENT_REQUIRES mcommon mespnow mwifi mupgrade mconfig json mdns esp_http_server vfs fatfs wpa_supplicant spiffs)

register_component()
