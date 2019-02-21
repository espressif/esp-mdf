#
# "main" pseudo-component makefile.
#
# (Uses default behaviour of compiling all source files in directory, adding 'include' to include path.)

COMPONENT_SRCDIRS += base_class button i2c_bus led

COMPONENT_ADD_INCLUDEDIRS += base_class/include
COMPONENT_ADD_INCLUDEDIRS += button/include
COMPONENT_ADD_INCLUDEDIRS += i2c_bus/include
COMPONENT_ADD_INCLUDEDIRS += led/include
