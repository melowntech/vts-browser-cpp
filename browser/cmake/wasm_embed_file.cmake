#
# Originally taken from:
# https://raw.githubusercontent.com/mosra/toolchains/master/modules/UseEmscripten.cmake
# and modified
#

function(wasm_embed_file target file destination)
    get_filename_component(absolute_file ${file} ABSOLUTE)
    get_target_property(${target}_LINK_FLAGS ${target} LINK_FLAGS)
    if(NOT ${target}_LINK_FLAGS)
        set(${target}_LINK_FLAGS )
    endif()
    set_target_properties(${target} PROPERTIES LINK_FLAGS "${${target}_LINK_FLAGS} --embed-file ${absolute_file}@${destination}")
endfunction()
