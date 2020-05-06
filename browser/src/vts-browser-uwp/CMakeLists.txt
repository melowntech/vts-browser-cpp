
define_module(BINARY vts-browser-uwp DEPENDS vts-browser vts-renderer)

set(SRC_LIST
  main.cpp
  Package.appxmanifest
)
add_executable(vts-browser-uwp ${SRC_LIST})
target_link_libraries(vts-browser-uwp ${MODULE_LIBRARIES})
target_compile_definitions(vts-browser-uwp PRIVATE ${MODULE_DEFINITIONS})
target_compile_options(vts-browser-uwp PRIVATE "/ZW") # enable microsoft extensions
target_compile_options(vts-browser-uwp PRIVATE "/Zc:twoPhase-") # disable two-phase lookup
buildsys_binary(vts-browser-uwp)
buildsys_ide_groups(vts-browser-uwp apps)

#set(PACKAGES_PATH "${CMAKE_CURRENT_BINARY_DIR}/packages")
## find or download nuget
#find_program(NUGET nuget)
#if(NOT EXISTS ${NUGET})
#  set(NUGET "${CMAKE_CURRENT_BINARY_DIR}/nuget.exe" CACHE FILEPATH "nuget" FORCE)
#  file(DOWNLOAD "https://dist.nuget.org/win-x86-commandline/latest/nuget.exe" "${NUGET}" STATUS status)
#  if(NOT EXISTS ${NUGET})
#    message(STATUS ${status})
#    message(FATAL_ERROR "Failed to download the nuget command line tool.")
#  endif()
#endif()
#
## run nuget to update packages at the beginning of the build
#add_custom_target(vts-browser-uwp-nuget
#  COMMAND ${NUGET} restore -PackagesDirectory ${PACKAGES_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/packages.config
#)
#add_dependencies(vts-browser-uwp vts-browser-uwp-nuget)
#buildsys_ide_groups(vts-browser-uwp-nuget apps)
#
#set_target_properties(vts-browser-uwp PROPERTIES VS_DOTNET_REFERENCE_ANGLE "${PACKAGES_PATH}/ANGLE.WindowsStore.2.1.13")
