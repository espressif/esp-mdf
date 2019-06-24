if(CONFIG_MESH_SUPPORT_ALIYUN_LINKKIT)
    set(COMPONENT_ADD_INCLUDEDIRS "include")
    set(COMPONENT_PRIV_INCLUDEDIRS ".")

    set(COMPONENT_SRCS "maliyun_linkkit_gateway.c"
                        "maliyun_linkkit_list.c"
                        "maliyun_linkkit_platform.c"
                        "maliyun_linkkit_subdevice.c")

else()
    message(STATUS "Building empty aws_iot component due to configuration")
endif()

# requirements can't depend on config
set(COMPONENT_REQUIRES esp-aliyun mcommon mwifi mlink mupgrade json)

register_component()