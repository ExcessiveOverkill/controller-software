cmake_minimum_required(VERSION 3.16.0)


# Setup compiler settings
set(CMAKE_C_STANDARD                11)
set(CMAKE_C_STANDARD_REQUIRED       ON)
set(CMAKE_C_EXTENSIONS              ON)
set(CMAKE_CXX_STANDARD              20)
set(CMAKE_CXX_STANDARD_REQUIRED     ON)
set(CMAKE_CXX_EXTENSIONS            ON)
set(PROJ_PATH                       ${CMAKE_CURRENT_SOURCE_DIR})
message("Build type: "              ${CMAKE_BUILD_TYPE})


project(controller_core)
enable_language(C CXX ASM)

set(EXECUTABLE controller_core)

#
# List of source files to compile
#
file(GLOB_RECURSE sources_SRCS
    ${PROJ_PATH}/controller/src/*.cpp
    ${PROJ_PATH}/controller/src/*.c
    ${PROJ_PATH}/controller/fpga_module_drivers/src/*.cpp
    ${PROJ_PATH}/controller/fpga_module_drivers/src/*.c
    ${PROJ_PATH}/controller/nodes/src/*.cpp
    ${PROJ_PATH}/controller/nodes/src/*.c
    ${PROJ_PATH}/controller/nodes/generic_nodes/src/*.cpp
    ${PROJ_PATH}/controller/nodes/generic_nodes/src/*.c
    ${PROJ_PATH}/controller/nodes/global_variables/src/*.cpp
    ${PROJ_PATH}/controller/nodes/global_variables/src/*.c
    ${PROJ_PATH}/controller/api_drivers/src/*.cpp
    ${PROJ_PATH}/controller/api_drivers/src/*.c
    ${PROJ_PATH}/controller/api_drivers/calls/src/*.cpp
    ${PROJ_PATH}/controller/api_drivers/calls/src/*.c

)

# Executable files
add_executable(${EXECUTABLE} ${sources_SRCS})

#
# Include directories
#
set(include_path_DIRS
    ${PROJ_PATH}/controller/inc
    ${PROJ_PATH}/controller/fpga_module_drivers/inc
    ${PROJ_PATH}/controller/nodes/inc
    ${PROJ_PATH}/controller/nodes/generic_nodes/inc
    ${PROJ_PATH}/controller/nodes/global_variables/inc
    ${PROJ_PATH}/controller/api_drivers/inc
    ${PROJ_PATH}/controller/api_drivers/calls/inc

)

# Include paths
target_include_directories(${EXECUTABLE} PRIVATE ${include_path_DIRS})
target_link_libraries(${EXECUTABLE} PRIVATE librt.so)
target_link_libraries(${EXECUTABLE} PRIVATE libpthread.so)

# copy all required files to the zynq directory
add_custom_command(TARGET controller_core POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:${EXECUTABLE}>
        ${CMAKE_CURRENT_SOURCE_DIR}/zynq_files/controller/bin
)

