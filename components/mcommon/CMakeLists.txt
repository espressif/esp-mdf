
set(COMPONENT_SRCS "mdf_err_to_name.c"
                    "mdf_event_loop.c"
                    "mdf_info_store.c"
                    "mdf_mem.c")

set(COMPONENT_INCLUDEDIRS "include")

idf_component_register(SRCS "${COMPONENT_SRCS}"
                    INCLUDE_DIRS "${COMPONENT_INCLUDEDIRS}"
                    REQUIRES spi_flash esp_http_client nvs_flash json)
