set(COMPONENT_ADD_INCLUDEDIRS include)

# Edit following two lines to set component requirements (see docs)
set(COMPONENT_REQUIRES mcommon)
set(COMPONENT_PRIV_REQUIRES )

set(COMPONENT_SRCS ./miniz.c)

register_component()
