
buildsys_fix_symlinks(${CMAKE_CURRENT_SOURCE_DIR})

option(UTF8_TESTS "Enable tests for UTF8-CPP" Off)
option(UTF8_INSTALL "Enable installation for UTF8-CPP" Off)
option(UTF8_SAMPLES "Enable building samples for UTF8-CPP" Off)

add_subdirectory(utfcpp)

