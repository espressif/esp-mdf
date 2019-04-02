set(COMPONENT_ADD_INCLUDEDIRS include)

# Edit following two lines to set component requirements (see docs)
set(COMPONENT_REQUIRES mcommon mbedtls mwifi mespnow)
set(COMPONENT_PRIV_REQUIRES )

set(COMPONENT_SRCS ./mconfig_security.c ./mconfig_queue.c ./mconfig_blufi.c ./mconfig_chain.c)

register_component()
