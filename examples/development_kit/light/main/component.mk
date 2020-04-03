#
# "main" pseudo-component makefile.
#
# (Uses default behaviour of compiling all source files in directory, adding 'include' to include path.)
COMPONENT_SRCDIRS := .

ifdef CONFIG_LIGHT_EXAMPLE_MESH
COMPONENT_OBJS := light_example.o
endif

ifdef CONFIG_LIGHT_EXAMPLE_ALIYUN
COMPONENT_OBJS := light_example.o
endif
