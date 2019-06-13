
set(COMPONENT_SRCS "light_example.c"
                    "util.c")

set(COMPONENT_ADD_INCLUDEDIRS ".")

# requirements can't depend on config
set(COMPONENT_REQUIRES mcommon mwifi mlink light_driver maliyun_linkkit esp-aliyun)

register_component()
