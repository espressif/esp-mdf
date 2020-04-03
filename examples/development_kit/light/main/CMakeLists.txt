
if(CONFIG_LIGHT_EXAMPLE_ALIYUN)
set(COMPONENT_SRCS "aliyun_light_example.c")
else()
set(COMPONENT_SRCS "light_example.c")
endif()

set(COMPONENT_ADD_INCLUDEDIRS ".")

# requirements can't depend on config
set(COMPONENT_REQUIRES mcommon mwifi mlink mdebug light_driver light_handle mesh_utils aliyun_sdk)
# if(CONFIG_LIGHT_EXAMPLE_ALIYUN)
# list(APPEND COMPONENT_REQUIRES aliyun_sdk)
# endif()

register_component()
