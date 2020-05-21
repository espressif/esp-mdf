
set(COMPONENT_SRCS "mconfig_chain.c"
                    "mconfig_queue.c"
                    "mconfig_security.c")

if(CONFIG_BT_ENABLED)
    set(COMPONENT_SRCS ${COMPONENT_SRCS}
                        "mconfig_blufi.c")
endif()

set(COMPONENT_INCLUDEDIRS "include")

idf_component_register(SRCS "${COMPONENT_SRCS}"
                    INCLUDE_DIRS "${COMPONENT_INCLUDEDIRS}"
                    REQUIRES bt mcommon mespnow mwifi miniz nvs_flash)
