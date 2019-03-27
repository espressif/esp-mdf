
set(COMPONENT_SRCS "mdf_err_to_name.c"
                    "mdf_event_loop.c"
                    "mdf_info_store.c"
                    "mdf_mem.c")

set(COMPONENT_ADD_INCLUDEDIRS "include")

# requirements can't depend on config
set(COMPONENT_REQUIRES spi_flash esp_http_client nvs_flash json)

register_component()
