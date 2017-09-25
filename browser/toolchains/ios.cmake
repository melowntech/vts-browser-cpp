
# Policies
cmake_policy(SET CMP0054 NEW)

# Standard settings
set(CMAKE_SYSTEM_NAME Darwin)
set(CMAKE_SYSTEM_VERSION 1)
set(UNIX TRUE)
set(APPLE TRUE)
set(IOS TRUE)

# Setup iOS platform unless specified manually with IOS_PLATFORM
if (NOT DEFINED IOS_PLATFORM)
  set(IOS_PLATFORM "iphonesimulator")
endif()

# Check the platform selection and setup for developer root
if (${IOS_PLATFORM} STREQUAL "iphoneos")
  set(CMAKE_XCODE_EFFECTIVE_PLATFORMS "-iphoneos")
  set(CMAKE_OSX_ARCHITECTURES arm64)
elseif (${IOS_PLATFORM} STREQUAL "iphonesimulator")
  set(CMAKE_XCODE_EFFECTIVE_PLATFORMS "-iphonesimulator")
  set(CMAKE_OSX_ARCHITECTURES x86_64)
else()
  message(FATAL_ERROR "Unsupported IOS_PLATFORM value selected. Please choose iphoneos or iphonesimulator")
endif()
set(CMAKE_OSX_ARCHITECTURES "${CMAKE_OSX_ARCHITECTURES}" CACHE STRING "osx architectures")

# Find compiler, ar and sysroot
execute_process(COMMAND xcode-select -print-path OUTPUT_VARIABLE CMAKE_XCODE_DEVELOPER_DIR OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND xcrun -sdk ${IOS_PLATFORM} -find gcc OUTPUT_VARIABLE CMAKE_C_COMPILER OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND xcrun -sdk ${IOS_PLATFORM} -find g++ OUTPUT_VARIABLE CMAKE_CXX_COMPILER OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND xcrun -sdk ${IOS_PLATFORM} -find ar OUTPUT_VARIABLE CMAKE_AR OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND xcrun -sdk ${IOS_PLATFORM} --show-sdk-path OUTPUT_VARIABLE CMAKE_OSX_SYSROOT OUTPUT_STRIP_TRAILING_WHITESPACE)
set(CMAKE_AR "${CMAKE_AR}" CACHE PATH "ar")
set(CMAKE_OSX_SYSROOT "${CMAKE_OSX_SYSROOT}" CACHE PATH "osx sysroot")

# All iOS/Darwin specific settings - some may be redundant
set(CMAKE_SHARED_LIBRARY_PREFIX "lib")
set(CMAKE_SHARED_LIBRARY_SUFFIX ".dylib")
set(CMAKE_SHARED_MODULE_PREFIX "lib")
set(CMAKE_SHARED_MODULE_SUFFIX ".so")
set(CMAKE_MODULE_EXISTS 1)
set(CMAKE_DL_LIBS "")

set(CMAKE_C_OSX_COMPATIBILITY_VERSION_FLAG "-compatibility_version ")
set(CMAKE_C_OSX_CURRENT_VERSION_FLAG "-current_version ")
set(CMAKE_CXX_OSX_COMPATIBILITY_VERSION_FLAG "${CMAKE_C_OSX_COMPATIBILITY_VERSION_FLAG}")
set(CMAKE_CXX_OSX_CURRENT_VERSION_FLAG "${CMAKE_C_OSX_CURRENT_VERSION_FLAG}")

set(CMAKE_C_FLAGS_INIT "")
set(CMAKE_CXX_FLAGS_INIT "-fvisibility=hidden -fvisibility-inlines-hidden")
set(CMAKE_C_FLAGS_INIT "${CMAKE_C_FLAGS_INIT}" CACHE STRING "c flags init")
set(CMAKE_CXX_FLAGS_INIT "${CMAKE_CXX_FLAGS_INIT}" CACHE STRING "c++ flags init")

set(CMAKE_C_LINK_FLAGS "-Wl,-search_paths_first ${CMAKE_C_LINK_FLAGS}")
set(CMAKE_CXX_LINK_FLAGS "-Wl,-search_paths_first ${CMAKE_CXX_LINK_FLAGS}")
set(CMAKE_C_LINK_FLAGS "${CMAKE_C_LINK_FLAGS}" CACHE STRING "c linker flags")
set(CMAKE_CXX_LINK_FLAGS "${CMAKE_CXX_LINK_FLAGS}" CACHE STRING "c++ linker flags")

set(CMAKE_PLATFORM_HAS_INSTALLNAME 1)
set(CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS "-dynamiclib -headerpad_max_install_names")
set(CMAKE_SHARED_MODULE_CREATE_C_FLAGS "-bundle -headerpad_max_install_names")
set(CMAKE_SHARED_MODULE_LOADER_C_FLAG "-Wl,-bundle_loader,")
set(CMAKE_SHARED_MODULE_LOADER_CXX_FLAG "-Wl,-bundle_loader,")
set(CMAKE_FIND_LIBRARY_SUFFIXES ".dylib" ".so" ".a")

# hack: overcome issue in try_compile for xcode generator
set(CMAKE_MACOSX_BUNDLE YES)
set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED "NO")

# Required as of cmake 2.8.10
set(CMAKE_OSX_DEPLOYMENT_TARGET "" CACHE STRING "Force unset of the deployment target for iOS" FORCE)

# hack: if a new cmake (which uses CMAKE_INSTALL_NAME_TOOL) runs on an old build tree
# (where install_name_tool was hardcoded) and where CMAKE_INSTALL_NAME_TOOL isn't in the cache
# and still cmake didn't fail in CMakeFindBinUtils.cmake (because it isn't rerun)
# hardcode CMAKE_INSTALL_NAME_TOOL here to install_name_tool, so it behaves as it did before, Alex
if (NOT DEFINED CMAKE_INSTALL_NAME_TOOL)
  find_program(CMAKE_INSTALL_NAME_TOOL install_name_tool)
endif()

# Set the find root to the iOS developer roots and to user defined paths
set(CMAKE_FIND_ROOT_PATH ${CMAKE_OSX_SYSROOT} ${CMAKE_IOS_SDK_ROOT} ${CMAKE_PREFIX_PATH} CACHE string  "iOS find search path root")

# default to searching for frameworks first
set(CMAKE_FIND_FRAMEWORK FIRST)

# set up the default search directories for frameworks
set(CMAKE_SYSTEM_FRAMEWORK_PATH
  ${CMAKE_IOS_SDK_ROOT}/System/Library/Frameworks
  ${CMAKE_IOS_SDK_ROOT}/System/Library/PrivateFrameworks
  ${CMAKE_IOS_SDK_ROOT}/Developer/Library/Frameworks
)

# only search the iOS sdks, not the remainder of the host filesystem
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)



# This little macro lets you set any XCode specific property
macro(set_xcode_property TARGET XCODE_PROPERTY XCODE_VALUE)
	set_property(TARGET ${TARGET} PROPERTY XCODE_ATTRIBUTE_${XCODE_PROPERTY} ${XCODE_VALUE})
endmacro(set_xcode_property)

# This macro lets you find executable programs on the host system
macro(find_host_package)
	set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
	set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY NEVER)
	set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE NEVER)
	set(IOS FALSE)
	find_package(${ARGN})
	set(IOS TRUE)
	set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM ONLY)
	set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
	set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
endmacro(find_host_package)


