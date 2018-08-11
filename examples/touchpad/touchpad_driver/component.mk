#
# "main" pseudo-component makefile.
#
# (Uses default behaviour of compiling all source files in directory, adding 'include' to include path.)

COMPONENT_SRCDIRS += light i2c_bus bh1750 touchpad led button base_class
COMPONENT_ADD_INCLUDEDIRS := light/include i2c_bus/include bh1750/include touchpad/include led/include button/include base_class/include