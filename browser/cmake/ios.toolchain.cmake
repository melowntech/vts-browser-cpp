
message(STATUS "***************************")
message(STATUS "*** Using ios toolchain ***")
message(STATUS "***************************")

# Policies
cmake_policy(SET CMP0054 NEW) # Only interpret if() arguments as variables or keywords when unquoted.

# Standard settings
set(CMAKE_CROSSCOMPILING TRUE)
set(CMAKE_SYSTEM_NAME Darwin)
set(CMAKE_SYSTEM_VERSION 1)
set(UNIX TRUE)
set(APPLE TRUE)
set(BUILDSYS_IOS TRUE)
set(BUILDSYS_EMBEDDED TRUE)

# Setup iOS platform unless specified manually with IOS_PLATFORM
if (NOT DEFINED IOS_PLATFORM)
    set(IOS_PLATFORM "iphoneos")
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
set(CMAKE_SYSTEM_PROCESSOR ${CMAKE_OSX_ARCHITECTURES})

# Find compiler, ar and sysroot
execute_process(COMMAND xcode-select -print-path OUTPUT_VARIABLE CMAKE_XCODE_DEVELOPER_DIR OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND xcrun -sdk ${IOS_PLATFORM} -find gcc OUTPUT_VARIABLE CMAKE_C_COMPILER OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND xcrun -sdk ${IOS_PLATFORM} -find g++ OUTPUT_VARIABLE CMAKE_CXX_COMPILER OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND xcrun -sdk ${IOS_PLATFORM} -find ar OUTPUT_VARIABLE CMAKE_AR OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND xcrun -sdk ${IOS_PLATFORM} --show-sdk-path OUTPUT_VARIABLE CMAKE_OSX_SYSROOT OUTPUT_STRIP_TRAILING_WHITESPACE)

set(CMAKE_XCODE_ATTRIBUTE_IPHONEOS_DEPLOYMENT_TARGET "10.0")
set(CMAKE_XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2")

set(common_flags "-fvisibility=hidden -fembed-bitcode")
set(CMAKE_C_FLAGS_INIT "${common_flags}")
set(CMAKE_CXX_FLAGS_INIT "${common_flags} -fvisibility-inlines-hidden")

# hack: overcome issue in try_compile for xcode generator
set(CMAKE_MACOSX_BUNDLE YES)
set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED "NO")

# Set the find root to the iOS developer roots and to user defined paths
set(CMAKE_FIND_ROOT_PATH ${CMAKE_OSX_SYSROOT} ${CMAKE_IOS_SDK_ROOT} ${CMAKE_PREFIX_PATH})

# default to searching for frameworks first
set(CMAKE_FIND_FRAMEWORK FIRST)

# set up the default search directories for frameworks
set(CMAKE_SYSTEM_FRAMEWORK_PATH
    ${CMAKE_IOS_SDK_ROOT}/System/Library/Frameworks
    ${CMAKE_IOS_SDK_ROOT}/System/Library/PrivateFrameworks
    ${CMAKE_IOS_SDK_ROOT}/Developer/Library/Frameworks
)

# what system to look on
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)


