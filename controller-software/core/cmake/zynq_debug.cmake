

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR arm)

# Specify the cross compiler
set(CMAKE_C_COMPILER arm-linux-gnueabihf-gcc)
set(CMAKE_CXX_COMPILER arm-linux-gnueabihf-g++)

# Specify the target architecture and NEON support
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mcpu=cortex-a9 -mfpu=neon -mfloat-abi=hard -g -O0")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-psabi -mcpu=cortex-a9 -mfpu=neon -mfloat-abi=hard -g -O0")

set(CMAKE_BUILD_TYPE Debug)

# Optionally specify the sysroot if necessary
# set(CMAKE_SYSROOT /path/to/sysroot)
