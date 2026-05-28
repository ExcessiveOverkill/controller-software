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
set(EDITOR_SERVICE_EXECUTABLE controller_editor_service)

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
    ${PROJ_PATH}/controller/nodes/solvers/src/*.cpp
    ${PROJ_PATH}/controller/nodes/solvers/src/*.c
    ${PROJ_PATH}/controller/api_drivers/src/*.cpp
    ${PROJ_PATH}/controller/api_drivers/src/*.c
    ${PROJ_PATH}/controller/api_drivers/calls/src/*.cpp
    ${PROJ_PATH}/controller/api_drivers/calls/src/*.c

)

# Executable files
add_executable(${EXECUTABLE} ${sources_SRCS})

# Standalone editor web service used for phased migration away from Node addon flow.
add_executable(${EDITOR_SERVICE_EXECUTABLE}
    ${PROJ_PATH}/editor_service/src/main.cpp
)

#
# Include directories
#
set(include_path_DIRS
    ${PROJ_PATH}/controller/inc
    ${PROJ_PATH}/controller/fpga_module_drivers/inc
    ${PROJ_PATH}/controller/nodes/inc
    ${PROJ_PATH}/controller/nodes/generic_nodes/inc
    ${PROJ_PATH}/controller/nodes/global_variables/inc
    ${PROJ_PATH}/controller/nodes/solvers/inc
    ${PROJ_PATH}/controller/api_drivers/inc
    ${PROJ_PATH}/controller/api_drivers/calls/inc

)

# Include paths
target_include_directories(${EXECUTABLE} PRIVATE ${include_path_DIRS})
target_include_directories(${EDITOR_SERVICE_EXECUTABLE} PRIVATE
    ${PROJ_PATH}/controller/inc
)
target_link_libraries(${EXECUTABLE} PRIVATE librt.so)
target_link_libraries(${EXECUTABLE} PRIVATE libpthread.so)
target_link_libraries(${EDITOR_SERVICE_EXECUTABLE} PRIVATE libpthread.so)

# copy all required files to the zynq directory
add_custom_command(TARGET controller_core POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:${EXECUTABLE}>
        ${CMAKE_CURRENT_SOURCE_DIR}/zynq_files/controller/bin/controller_core
)

add_custom_command(TARGET ${EDITOR_SERVICE_EXECUTABLE} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:${EDITOR_SERVICE_EXECUTABLE}>
        ${CMAKE_CURRENT_SOURCE_DIR}/zynq_files/controller/bin/controller_editor_service
)

