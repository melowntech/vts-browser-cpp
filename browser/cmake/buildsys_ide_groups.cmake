
function(buildsys_ide_groups target folder)
    set_property(GLOBAL PROPERTY USE_FOLDERS ON)
    set_target_properties(${target} PROPERTIES FOLDER ${folder})
    # todo check for files that are outside the prefix
    #if(${CMAKE_VERSION} VERSION_GREATER 3.8)
    #    get_property(files TARGET ${target} PROPERTY SOURCES)
    #    source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${files})
    #endif()
endfunction(buildsys_ide_groups)

