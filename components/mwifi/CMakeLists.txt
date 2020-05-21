
set(COMPONENT_SRCS "mwifi.c")

set(COMPONENT_INCLUDEDIRS "include")

idf_component_register(SRCS "${COMPONENT_SRCS}"
                    INCLUDE_DIRS "${COMPONENT_INCLUDEDIRS}"
                    REQUIRES mcommon miniz)
