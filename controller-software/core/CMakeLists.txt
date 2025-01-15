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
)

# Executable files
add_executable(${EXECUTABLE} ${sources_SRCS})

#
# Include directories
#
set(include_path_DIRS
    ${PROJ_PATH}/controller/inc
    ${PROJ_PATH}/controller/fpga_module_drivers/inc
)

# Include paths
target_include_directories(${EXECUTABLE} PRIVATE ${include_path_DIRS})

# copy all required files to the zynq directory
add_custom_command(TARGET controller_core POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:${EXECUTABLE}>
        ${CMAKE_CURRENT_SOURCE_DIR}/zynq_files/controller/bin
)