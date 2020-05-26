
idf_component_register(
    SRCS "./epaper_font.c" "./epaper.c"
    INCLUDE_DIRS "." "./include"
    REQUIRES mcommon mwifi mlink i2c_bus
    )
