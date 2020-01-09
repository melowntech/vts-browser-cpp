
message(STATUS "***************************")
message(STATUS "*** Using IOS toolchain ***")
message(STATUS "***************************")

# standard settings
set(CMAKE_CROSSCOMPILING TRUE)
set(BUILDSYS_IOS TRUE)
set(BUILDSYS_EMBEDDED TRUE)

# setup iOS platform unless specified manually with IOS_PLATFORM
if(NOT DEFINED IOS_PLATFORM)
    set(IOS_PLATFORM "iphoneos")
endif()
# check the platform selection and setup for developer root
if(IOS_PLATFORM STREQUAL "iphoneos")
    set(CMAKE_XCODE_EFFECTIVE_PLATFORMS "-iphoneos")
    set(CMAKE_OSX_ARCHITECTURES arm64)
elseif(IOS_PLATFORM STREQUAL "iphonesimulator")
    set(CMAKE_XCODE_EFFECTIVE_PLATFORMS "-iphonesimulator")
    set(CMAKE_OSX_ARCHITECTURES x86_64)
else()
    message(FATAL_ERROR "Unsupported IOS_PLATFORM value selected. Please choose iphoneos or iphonesimulator")
endif()
set(CMAKE_SYSTEM_PROCESSOR ${CMAKE_OSX_ARCHITECTURES})
set(CMAKE_XCODE_ATTRIBUTE_ARCHS ${CMAKE_OSX_ARCHITECTURES})

# find compiler, ar and sysroot
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
set(CMAKE_EXECUTABLE_RUNTIME_CXX_FLAG -Wl,-rpath,)
set(CMAKE_EXECUTABLE_RUNTIME_C_FLAG -Wl,-rpath,)
set(CMAKE_SHARED_LIBRARY_RUNTIME_CXX_FLAG -Wl,-rpath,)
set(CMAKE_SHARED_LIBRARY_RUNTIME_C_FLAG -Wl,-rpath,)

# hack: overcome issue in try_compile for xcode generator
set(CMAKE_MACOSX_BUNDLE TRUE)
set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED "NO")
set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "")

# set the find root to the iOS developer roots and to user defined paths
set(CMAKE_FIND_ROOT_PATH ${CMAKE_OSX_SYSROOT} ${CMAKE_IOS_SDK_ROOT} ${CMAKE_PREFIX_PATH})

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
set(CMAKE_FIND_FRAMEWORK FIRST)

# rpath options
set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)


