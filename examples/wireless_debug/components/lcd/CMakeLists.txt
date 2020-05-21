
# componet standalone mode
if(NOT CONFIG_IOT_SOLUTION_EMBED)
    set(srcs "adaptation.cpp"
                        "font7s.c"
                        "glcdfont.c"
                        "iot_lcd.cpp"
                        "spi_lcd.c"
                        "Adafruit-GFX-Library/Adafruit_GFX.cpp"
                        # "Adafruit-GFX-Library/Adafruit_SPITFT.cpp"
                        "Adafruit-GFX-Library/glcdfont.c"
    )
    set(include_dirs "."
                                    "include"
                                    "Adafruit-GFX-Library/Fonts"
                                    "Adafruit-GFX-Library")
else()
    if(CONFIG_IOT_LCD_ENABLE)
        set(srcs "adaptation.cpp"
                            "font7s.c"
                            "glcdfont.c"
                            "iot_lcd.cpp"
                            "spi_lcd.c"
                            "Adafruit-GFX-Library/Adafruit_GFX.cpp"
                            # "Adafruit-GFX-Library/Adafruit_SPITFT.cpp"
                            "Adafruit-GFX-Library/glcdfont.c")

        set(include_dirs "."
                                        "include"
                                        "Adafruit-GFX-Library/Fonts"
                                        "Adafruit-GFX-Library")
    else()
        set(srcs "")
        set(include_dirs "")
        message(STATUS "Building empty lcd component due to configuration")
    endif()
endif()

# requirements can't depend on config
idf_component_register(SRCS ${srcs}
                    INCLUDE_DIRS ${include_dirs}
                    REQUIRES spi_flash
)

target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-unused-const-variable)
add_compile_options(-D__AVR_ATtiny85__)
