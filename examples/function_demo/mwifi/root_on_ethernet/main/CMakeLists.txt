
set(COMPONENT_SRCS 
    "./ethernet_example.c"
    "./eth_helper.c")

set(COMPONENT_ADD_INCLUDEDIRS ".")

# requirements can't depend on config
set(COMPONENT_REQUIRES mcommon mconfig mwifi mlink ethernet)

register_component()
