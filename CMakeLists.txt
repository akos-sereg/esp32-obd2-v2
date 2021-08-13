# The following four lines of boilerplate have to be in your project's CMakeLists
# in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)
set(EXTRA_COMPONENT_DIRS c:/projects/esp32/esp-idf-lib/esp-idf-lib/components)
set(EXTRA_COMPONENT_DIRS c:/projects/esp32/esp32-obd2-v2)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(esp32-obd2-v2)
