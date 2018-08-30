
function(buildsys_ide_groups target folder)
    # place the target into specified folder
    set_property(GLOBAL PROPERTY USE_FOLDERS ON)
    set_target_properties(${target} PROPERTIES FOLDER ${folder})

    # rearrange files for the target to match the filesystem
    if(${CMAKE_VERSION} VERSION_GREATER 3.8)
        get_property(dir TARGET ${target} PROPERTY SOURCE_DIR)
        string(LENGTH "${dir}" dirlen)
        get_property(files TARGET ${target} PROPERTY SOURCES)
        # filter out files that cannot be rearranged
        foreach(f IN LISTS files)
            get_filename_component(fa "${f}" ABSOLUTE BASE_DIR "${dir}")
            string(FIND "${fa}" "${dir}" ok)
            if(ok EQUAL 0)
                get_filename_component(g "${fa}" DIRECTORY)
                string(SUBSTRING "${g}" ${dirlen} -1 g)
                if(NOT g)
                    set(g "/")
                endif()
                string(REPLACE "/" "\\" g "${g}")
                source_group("${g}" FILES "${fa}")
            endif()
        endforeach()
    endif()
endfunction(buildsys_ide_groups)

