
message(STATUS "***************************")
message(STATUS "*** Using UWP toolchain ***")
message(STATUS "***************************")

# standard settings
set(CMAKE_CROSSCOMPILING TRUE)
set(BUILDSYS_UWP TRUE)
set(BUILDSYS_EMBEDDED TRUE)

set(CMAKE_SYSTEM_NAME WindowsStore)
set(CMAKE_SYSTEM_VERSION 10.0)
if(NOT CMAKE_SYSTEM_PROCESSOR)
    set(CMAKE_SYSTEM_PROCESSOR x86_64)
endif()


