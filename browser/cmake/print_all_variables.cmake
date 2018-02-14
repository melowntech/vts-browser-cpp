function(print_all_variables)
    message(STATUS "---- cmake variables ----")
    get_cmake_property(vs VARIABLES)
    foreach(v ${vs})
        message(STATUS "${v} = '${${v}}'")
    endforeach()
    message(STATUS "---- cmake variables ----")
endfunction()

