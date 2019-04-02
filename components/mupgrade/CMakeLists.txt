set(COMPONENT_ADD_INCLUDEDIRS include)

# Edit following two lines to set component requirements (see docs)
set(COMPONENT_REQUIRES mcommon app_update mwifi)
set(COMPONENT_PRIV_REQUIRES )

set(COMPONENT_SRCS ./mupgrade_node.c ./mupgrade_root.c ./mupgrade_check.c)

register_component()
