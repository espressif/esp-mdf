IDF_PATH := $(MDF_PATH)/esp-idf

EXTRA_COMPONENT_DIRS += $(MDF_PATH)/components/
EXTRA_COMPONENT_DIRS += $(MDF_PATH)/components/protocol_stacks
EXTRA_COMPONENT_DIRS += $(MDF_PATH)/components/base_components
EXTRA_COMPONENT_DIRS += $(MDF_PATH)/components/functions
EXTRA_COMPONENT_DIRS += $(MDF_PATH)/components/device_handle

include $(IDF_PATH)/make/project.mk
