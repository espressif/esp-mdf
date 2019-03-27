# Some MDF-specific functions and functions

# mdf_get_git_revision
#
# Set global MDF_VER to the git revision of ESP-MDF.
#
# Running git_describe() here automatically triggers rebuilds
# if the ESP-MDF git version changes
function(mdf_get_git_revision)
    if(EXISTS "$ENV{MDF_PATH}/version.txt")
        file(STRINGS "$ENV{MDF_PATH}/version.txt" MDF_VER)
    else()
        git_describe(MDF_VER "$ENV{MDF_PATH}")
    endif()
    add_definitions(-DMDF_VER=\"${MDF_VER}\")
    git_submodule_check("$ENV{MDF_PATH}")
    set(MDF_VER ${MDF_VER} PARENT_SCOPE)
endfunction()
