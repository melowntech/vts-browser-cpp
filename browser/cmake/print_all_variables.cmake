
function(buildsys_print_all_variables)
    message(STATUS "----------------------------------------------------------------")
    message(STATUS "print all variables begin")
    message(STATUS "----------------------------------------------------------------")
    get_cmake_property(variableNames VARIABLES)
    foreach(variableName ${variableNames})
        message(STATUS "${variableName} = ${${variableName}}")
    endforeach()
    message(STATUS "----------------------------------------------------------------")
    message(STATUS "print all variables end")
    message(STATUS "----------------------------------------------------------------")
endfunction()

