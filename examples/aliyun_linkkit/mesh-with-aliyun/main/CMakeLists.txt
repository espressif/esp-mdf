
set(COMPONENT_SRCS "./mesh_with_aliyun.c")

set(COMPONENT_ADD_INCLUDEDIRS "")

# requirements can't depend on config
set(COMPONENT_REQUIRES mcommon mconfig mwifi mlink mupgrade aliyun_sdk)

register_component()
