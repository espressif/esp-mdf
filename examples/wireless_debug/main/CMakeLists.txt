
set(COMPONENT_SRCS "main.c" 
                   "wifi_sniffer_cmd.c"
                   "debug_cmd.c"
                   "debug_recv.cpp")

set(COMPONENT_ADD_INCLUDEDIRS ". include")

# requirements can't depend on config
set(COMPONENT_REQUIRES mcommon mwifi mdebug i2c_bus lcd pcap sdcard)

register_component()
