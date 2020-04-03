#
# This is a project Makefile for the test subproject.
#

PROJECT_NAME := aliyun_unit_test

# Include the components directory of the main application:
#
EXTRA_COMPONENT_DIRS := $(realpath ../../../components)

# Set the components to include the tests for.
# This can be overriden from the command line
# (e.g. 'make TEST_COMPONENTS=xxxx flash monitor')
#
TEST_COMPONENTS ?= aliyun_sdk

include $(MDF_PATH)/project.mk
