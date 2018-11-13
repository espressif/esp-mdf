IDF_PATH := $(MDF_PATH)/esp-idf

MDF_VER := $(shell cd ${MDF_PATH} && git describe --always --tags --dirty)
CPPFLAGS := -D MDF_VER=\"$(MDF_VER)\"

EXTRA_COMPONENT_DIRS += $(MDF_PATH)/components/
EXTRA_COMPONENT_DIRS += $(MDF_PATH)/components/third_party

include $(IDF_PATH)/make/project.mk
