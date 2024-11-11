cmake_minimum_required(VERSION 3.16.0)

project(controller_core VERSION 0.1.0 LANGUAGES C CXX)

add_executable(controller_core main.cpp)

# copy all required files to the zynq directory
add_custom_command(TARGET controller_core POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/build/controller_core
        #${CMAKE_CURRENT_SOURCE_DIR}/file2.txt
        ${CMAKE_CURRENT_SOURCE_DIR}/zynq_files/controller/bin
)