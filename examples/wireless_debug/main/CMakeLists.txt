
idf_component_register(SRCS "./main.c" "./wifi_sniffer_cmd.c" "./debug_cmd.c" "./debug_recv.cpp"
                    INCLUDE_DIRS "." "./include"
                    REQUIRES mcommon mwifi mdebug i2c_bus lcd pcap sdcard
)
