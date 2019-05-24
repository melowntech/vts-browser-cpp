
message(STATUS "using dummy Optick")

add_library(Optick INTERFACE)
target_include_directories(Optick INTERFACE .)
