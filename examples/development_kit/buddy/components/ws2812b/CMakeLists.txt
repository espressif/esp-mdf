idf_component_register(SRCS "./ws2812b.c"
                    INCLUDE_DIRS "." "./include"
                    REQUIRES mcommon mwifi mconfig
)