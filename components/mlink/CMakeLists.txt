set(COMPONENT_ADD_INCLUDEDIRS include)

# Edit following two lines to set component requirements (see docs)
set(COMPONENT_REQUIRES mcommon mdns mespnow mwifi mconfig esp_http_server mupgrade)
set(COMPONENT_PRIV_REQUIRES )

set(COMPONENT_SRCS ./mlink_notice.c ./mlink_utils.c ./mlink_espnow.c ./mlink_trigger.c ./mlink_json.c ./mlink_httpd.c ./mlink_handle.c)

register_component()
