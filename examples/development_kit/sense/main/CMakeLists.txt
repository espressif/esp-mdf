
idf_component_register(SRCS "./sense_epaper.cpp"
                            "./sense_led.cpp"
                            "./sense_main.c"
                            "./sense_sensors.cpp"
                    INCLUDE_DIRS "include"
                    REQUIRES    mcommon 
                                mwifi 
                                mlink 
                                epaper 
                                hts221 
                                bh1750 
                                ulp_operation 
                                base_class 
                                led 
                                button
                    )

