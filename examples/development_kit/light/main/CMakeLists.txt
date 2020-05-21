
if(CONFIG_LIGHT_EXAMPLE_ALIYUN)
set(COMPONENT_SRCS "aliyun_light_example.c")
set(COMPONENT_REQUIRES mcommon mwifi mlink mdebug light_driver light_handle mesh_utils aliyun_sdk)
else()
set(COMPONENT_SRCS "light_example.c")
set(COMPONENT_REQUIRES mcommon mwifi mlink mdebug light_driver light_handle mesh_utils)
endif()

idf_component_register(SRCS "./light_example.c"
                    INCLUDE_DIRS "."
                    REQUIRES mcommon mwifi mlink mdebug light_driver light_handle mesh_utils
)
