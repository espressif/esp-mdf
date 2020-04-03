
set(COMPONENT_SRCS "get_started.c")

# requirements can't depend on config
set(COMPONENT_REQUIRES aliyun_sdk mcommon mwifi)

register_component()
