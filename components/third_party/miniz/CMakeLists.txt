
set(COMPONENT_SRCS miniz.c miniz_tinfl.c miniz_tdef.c)
set(COMPONENT_INCLUDEDIRS ".")

idf_component_register(SRCS "${COMPONENT_SRCS}"
                    INCLUDE_DIRS "${COMPONENT_INCLUDEDIRS}"
                    )


