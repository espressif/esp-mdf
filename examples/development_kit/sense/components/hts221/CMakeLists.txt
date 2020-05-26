
idf_component_register(
    SRCS "./hts221.c" "./hts221_obj.cpp"
    INCLUDE_DIRS "." "./include"
    REQUIRES mcommon mwifi mlink i2c_bus
    )
