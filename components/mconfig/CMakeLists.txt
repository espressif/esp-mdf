
set(COMPONENT_SRCS "mconfig_chain.c"
                    "mconfig_queue.c"
                    "mconfig_security.c")

if(CONFIG_BT_ENABLED)
    set(COMPONENT_SRCS ${COMPONENT_SRCS}
                        "mconfig_blufi.c")
endif()

set(COMPONENT_ADD_INCLUDEDIRS "include")

# requirements can't depend on config
set(COMPONENT_REQUIRES bt mcommon mespnow mwifi miniz nvs_flash)

register_component()
