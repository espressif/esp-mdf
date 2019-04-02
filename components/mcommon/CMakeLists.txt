set(COMPONENT_ADD_INCLUDEDIRS include)

# Edit following two lines to set component requirements (see docs)
set(COMPONENT_REQUIRES spi_flash esp_http_client nvs_flash json bt)
set(COMPONENT_PRIV_REQUIRES )

set(COMPONENT_SRCS ./mdf_event_loop.c ./mdf_err_to_name.c ./mdf_info_store.c ./mdf_mem.c)

register_component()
