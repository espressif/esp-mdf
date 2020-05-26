
idf_component_register(
    SRCS "./led.c" "./led_obj.cpp"
    INCLUDE_DIRS "." "./include"
    REQUIRES mcommon mwifi mlink base_class
    )
