# Designed to be included from an IDF app's CMakeLists.txt file
#
cmake_minimum_required(VERSION 3.5)

# Set MDF_PATH, as nothing else will work without this
set(MDF_PATH "$ENV{MDF_PATH}")
if(NOT MDF_PATH)
    # Documentation says you should set MDF_PATH in your environment, but we
    # can infer it relative to current directory if it's not set.
    get_filename_component(MDF_PATH "${CMAKE_CURRENT_LIST_DIR}" ABSOLUTE)
endif()
file(TO_CMAKE_PATH "${MDF_PATH}" MDF_PATH)
set(ENV{MDF_PATH} ${MDF_PATH})

# set(ENV{IDF_PATH} "$ENV{MDF_PATH}/esp-idf/")

set(EXTRA_COMPONENT_DIRS "${EXTRA_COMPONENT_DIRS} $ENV{MDF_PATH}/components/")
set(EXTRA_COMPONENT_DIRS "${EXTRA_COMPONENT_DIRS} $ENV{MDF_PATH}/components/third_party")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
include($ENV{MDF_PATH}/mdf_functions.cmake)

# Check git revision (may trigger reruns of cmake)
##  sets MDF_VER to MDF git revision
mdf_get_git_revision()
