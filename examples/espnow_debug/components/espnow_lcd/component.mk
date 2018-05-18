#
# "main" pseudo-component makefile.
#
# (Uses default behaviour of compiling all source files in directory, adding 'include' to include path.)

CXXFLAGS += -D__AVR_ATtiny85__

COMPONENT_SRCDIRS += lcd iot_lcd iot_lcd/Adafruit-GFX-Library
COMPONENT_ADD_INCLUDEDIRS := lcd/include iot_lcd/include iot_lcd/Adafruit-GFX-Library iot_lcd/Adafruit-GFX-Library/Fonts
