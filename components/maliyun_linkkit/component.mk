#
# Component Makefile
#

ifdef CONFIG_MESH_SUPPORT_ALIYUN_LINKKIT

COMPONENT_ADD_INCLUDEDIRS := include

COMPONENT_SRCDIRS := .

# Check the submodule is initialised
#COMPONENT_SUBMODULES :=

else
# Disable maliyun linkkit support
COMPONENT_ADD_INCLUDEDIRS :=
COMPONENT_ADD_LDFLAGS :=
COMPONENT_SRCDIRS :=
endif
