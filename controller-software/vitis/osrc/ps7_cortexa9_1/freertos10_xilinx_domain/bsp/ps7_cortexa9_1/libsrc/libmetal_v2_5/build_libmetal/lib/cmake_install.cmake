# Install script for directory: /home/excessive/github/controller-software/controller-software/vitis/osrc/ps7_cortexa9_1/freertos10_xilinx_domain/bsp/ps7_cortexa9_1/libsrc/libmetal_v2_5/src/libmetal/lib

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

if(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include" TYPE FILE RENAME "metal/alloc.h" FILES "/home/excessive/github/controller-software/controller-software/vitis/osrc/ps7_cortexa9_1/freertos10_xilinx_domain/bsp/ps7_cortexa9_1/libsrc/libmetal_v2_5/build_libmetal/lib/include/metal/alloc.h")
endif()

if(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include" TYPE FILE RENAME "metal/assert.h" FILES "/home/excessive/github/controller-software/controller-software/vitis/osrc/ps7_cortexa9_1/freertos10_xilinx_domain/bsp/ps7_cortexa9_1/libsrc/libmetal_v2_5/build_libmetal/lib/include/metal/assert.h")
endif()

if(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include" TYPE FILE RENAME "metal/atomic.h" FILES "/home/excessive/github/controller-software/controller-software/vitis/osrc/ps7_cortexa9_1/freertos10_xilinx_domain/bsp/ps7_cortexa9_1/libsrc/libmetal_v2_5/build_libmetal/lib/include/metal/atomic.h")
endif()

if(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include" TYPE FILE RENAME "metal/cache.h" FILES "/home/excessive/github/controller-software/controller-software/vitis/osrc/ps7_cortexa9_1/freertos10_xilinx_domain/bsp/ps7_cortexa9_1/libsrc/libmetal_v2_5/build_libmetal/lib/include/metal/cache.h")
endif()

if(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include" TYPE FILE RENAME "metal/compiler.h" FILES "/home/excessive/github/controller-software/controller-software/vitis/osrc/ps7_cortexa9_1/freertos10_xilinx_domain/bsp/ps7_cortexa9_1/libsrc/libmetal_v2_5/build_libmetal/lib/include/metal/compiler.h")
endif()

if(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include" TYPE FILE RENAME "metal/condition.h" FILES "/home/excessive/github/controller-software/controller-software/vitis/osrc/ps7_cortexa9_1/freertos10_xilinx_domain/bsp/ps7_cortexa9_1/libsrc/libmetal_v2_5/build_libmetal/lib/include/metal/condition.h")
endif()

if(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include" TYPE FILE RENAME "metal/config.h" FILES "/home/excessive/github/controller-software/controller-software/vitis/osrc/ps7_cortexa9_1/freertos10_xilinx_domain/bsp/ps7_cortexa9_1/libsrc/libmetal_v2_5/build_libmetal/lib/include/metal/config.h")
endif()

if(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include" TYPE FILE RENAME "metal/cpu.h" FILES "/home/excessive/github/controller-software/controller-software/vitis/osrc/ps7_cortexa9_1/freertos10_xilinx_domain/bsp/ps7_cortexa9_1/libsrc/libmetal_v2_5/build_libmetal/lib/include/metal/cpu.h")
endif()

if(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include" TYPE FILE RENAME "metal/device.h" FILES "/home/excessive/github/controller-software/controller-software/vitis/osrc/ps7_cortexa9_1/freertos10_xilinx_domain/bsp/ps7_cortexa9_1/libsrc/libmetal_v2_5/build_libmetal/lib/include/metal/device.h")
endif()

if(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include" TYPE FILE RENAME "metal/errno.h" FILES "/home/excessive/github/controller-software/controller-software/vitis/osrc/ps7_cortexa9_1/freertos10_xilinx_domain/bsp/ps7_cortexa9_1/libsrc/libmetal_v2_5/build_libmetal/lib/include/metal/errno.h")
endif()

if(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include" TYPE FILE RENAME "metal/dma.h" FILES "/home/excessive/github/controller-software/controller-software/vitis/osrc/ps7_cortexa9_1/freertos10_xilinx_domain/bsp/ps7_cortexa9_1/libsrc/libmetal_v2_5/build_libmetal/lib/include/metal/dma.h")
endif()

if(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include" TYPE FILE RENAME "metal/io.h" FILES "/home/excessive/github/controller-software/controller-software/vitis/osrc/ps7_cortexa9_1/freertos10_xilinx_domain/bsp/ps7_cortexa9_1/libsrc/libmetal_v2_5/build_libmetal/lib/include/metal/io.h")
endif()

if(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include" TYPE FILE RENAME "metal/irq.h" FILES "/home/excessive/github/controller-software/controller-software/vitis/osrc/ps7_cortexa9_1/freertos10_xilinx_domain/bsp/ps7_cortexa9_1/libsrc/libmetal_v2_5/build_libmetal/lib/include/metal/irq.h")
endif()

if(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include" TYPE FILE RENAME "metal/irq_controller.h" FILES "/home/excessive/github/controller-software/controller-software/vitis/osrc/ps7_cortexa9_1/freertos10_xilinx_domain/bsp/ps7_cortexa9_1/libsrc/libmetal_v2_5/build_libmetal/lib/include/metal/irq_controller.h")
endif()

if(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include" TYPE FILE RENAME "metal/list.h" FILES "/home/excessive/github/controller-software/controller-software/vitis/osrc/ps7_cortexa9_1/freertos10_xilinx_domain/bsp/ps7_cortexa9_1/libsrc/libmetal_v2_5/build_libmetal/lib/include/metal/list.h")
endif()

if(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include" TYPE FILE RENAME "metal/log.h" FILES "/home/excessive/github/controller-software/controller-software/vitis/osrc/ps7_cortexa9_1/freertos10_xilinx_domain/bsp/ps7_cortexa9_1/libsrc/libmetal_v2_5/build_libmetal/lib/include/metal/log.h")
endif()

if(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include" TYPE FILE RENAME "metal/mutex.h" FILES "/home/excessive/github/controller-software/controller-software/vitis/osrc/ps7_cortexa9_1/freertos10_xilinx_domain/bsp/ps7_cortexa9_1/libsrc/libmetal_v2_5/build_libmetal/lib/include/metal/mutex.h")
endif()

if(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include" TYPE FILE RENAME "metal/shmem.h" FILES "/home/excessive/github/controller-software/controller-software/vitis/osrc/ps7_cortexa9_1/freertos10_xilinx_domain/bsp/ps7_cortexa9_1/libsrc/libmetal_v2_5/build_libmetal/lib/include/metal/shmem.h")
endif()

if(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include" TYPE FILE RENAME "metal/sleep.h" FILES "/home/excessive/github/controller-software/controller-software/vitis/osrc/ps7_cortexa9_1/freertos10_xilinx_domain/bsp/ps7_cortexa9_1/libsrc/libmetal_v2_5/build_libmetal/lib/include/metal/sleep.h")
endif()

if(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include" TYPE FILE RENAME "metal/softirq.h" FILES "/home/excessive/github/controller-software/controller-software/vitis/osrc/ps7_cortexa9_1/freertos10_xilinx_domain/bsp/ps7_cortexa9_1/libsrc/libmetal_v2_5/build_libmetal/lib/include/metal/softirq.h")
endif()

if(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include" TYPE FILE RENAME "metal/spinlock.h" FILES "/home/excessive/github/controller-software/controller-software/vitis/osrc/ps7_cortexa9_1/freertos10_xilinx_domain/bsp/ps7_cortexa9_1/libsrc/libmetal_v2_5/build_libmetal/lib/include/metal/spinlock.h")
endif()

if(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include" TYPE FILE RENAME "metal/sys.h" FILES "/home/excessive/github/controller-software/controller-software/vitis/osrc/ps7_cortexa9_1/freertos10_xilinx_domain/bsp/ps7_cortexa9_1/libsrc/libmetal_v2_5/build_libmetal/lib/include/metal/sys.h")
endif()

if(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include" TYPE FILE RENAME "metal/time.h" FILES "/home/excessive/github/controller-software/controller-software/vitis/osrc/ps7_cortexa9_1/freertos10_xilinx_domain/bsp/ps7_cortexa9_1/libsrc/libmetal_v2_5/build_libmetal/lib/include/metal/time.h")
endif()

if(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include" TYPE FILE RENAME "metal/utilities.h" FILES "/home/excessive/github/controller-software/controller-software/vitis/osrc/ps7_cortexa9_1/freertos10_xilinx_domain/bsp/ps7_cortexa9_1/libsrc/libmetal_v2_5/build_libmetal/lib/include/metal/utilities.h")
endif()

if(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include" TYPE FILE RENAME "metal/version.h" FILES "/home/excessive/github/controller-software/controller-software/vitis/osrc/ps7_cortexa9_1/freertos10_xilinx_domain/bsp/ps7_cortexa9_1/libsrc/libmetal_v2_5/build_libmetal/lib/include/metal/version.h")
endif()

if(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include" TYPE FILE RENAME "metal/compiler/armcc/errno.h" FILES "/home/excessive/github/controller-software/controller-software/vitis/osrc/ps7_cortexa9_1/freertos10_xilinx_domain/bsp/ps7_cortexa9_1/libsrc/libmetal_v2_5/build_libmetal/lib/include/metal/compiler/armcc/errno.h")
endif()

if(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include" TYPE FILE RENAME "metal/compiler/gcc/atomic.h" FILES "/home/excessive/github/controller-software/controller-software/vitis/osrc/ps7_cortexa9_1/freertos10_xilinx_domain/bsp/ps7_cortexa9_1/libsrc/libmetal_v2_5/build_libmetal/lib/include/metal/compiler/gcc/atomic.h")
endif()

if(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include" TYPE FILE RENAME "metal/compiler/gcc/compiler.h" FILES "/home/excessive/github/controller-software/controller-software/vitis/osrc/ps7_cortexa9_1/freertos10_xilinx_domain/bsp/ps7_cortexa9_1/libsrc/libmetal_v2_5/build_libmetal/lib/include/metal/compiler/gcc/compiler.h")
endif()

if(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include" TYPE FILE RENAME "metal/compiler/iar/compiler.h" FILES "/home/excessive/github/controller-software/controller-software/vitis/osrc/ps7_cortexa9_1/freertos10_xilinx_domain/bsp/ps7_cortexa9_1/libsrc/libmetal_v2_5/build_libmetal/lib/include/metal/compiler/iar/compiler.h")
endif()

if(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include" TYPE FILE RENAME "metal/compiler/iar/errno.h" FILES "/home/excessive/github/controller-software/controller-software/vitis/osrc/ps7_cortexa9_1/freertos10_xilinx_domain/bsp/ps7_cortexa9_1/libsrc/libmetal_v2_5/build_libmetal/lib/include/metal/compiler/iar/errno.h")
endif()

if(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include" TYPE FILE RENAME "metal/processor/arm/atomic.h" FILES "/home/excessive/github/controller-software/controller-software/vitis/osrc/ps7_cortexa9_1/freertos10_xilinx_domain/bsp/ps7_cortexa9_1/libsrc/libmetal_v2_5/build_libmetal/lib/include/metal/processor/arm/atomic.h")
endif()

if(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include" TYPE FILE RENAME "metal/processor/arm/cpu.h" FILES "/home/excessive/github/controller-software/controller-software/vitis/osrc/ps7_cortexa9_1/freertos10_xilinx_domain/bsp/ps7_cortexa9_1/libsrc/libmetal_v2_5/build_libmetal/lib/include/metal/processor/arm/cpu.h")
endif()

if(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include" TYPE FILE RENAME "metal/system/freertos/alloc.h" FILES "/home/excessive/github/controller-software/controller-software/vitis/osrc/ps7_cortexa9_1/freertos10_xilinx_domain/bsp/ps7_cortexa9_1/libsrc/libmetal_v2_5/build_libmetal/lib/include/metal/system/freertos/alloc.h")
endif()

if(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include" TYPE FILE RENAME "metal/system/freertos/assert.h" FILES "/home/excessive/github/controller-software/controller-software/vitis/osrc/ps7_cortexa9_1/freertos10_xilinx_domain/bsp/ps7_cortexa9_1/libsrc/libmetal_v2_5/build_libmetal/lib/include/metal/system/freertos/assert.h")
endif()

if(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include" TYPE FILE RENAME "metal/system/freertos/cache.h" FILES "/home/excessive/github/controller-software/controller-software/vitis/osrc/ps7_cortexa9_1/freertos10_xilinx_domain/bsp/ps7_cortexa9_1/libsrc/libmetal_v2_5/build_libmetal/lib/include/metal/system/freertos/cache.h")
endif()

if(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include" TYPE FILE RENAME "metal/system/freertos/condition.h" FILES "/home/excessive/github/controller-software/controller-software/vitis/osrc/ps7_cortexa9_1/freertos10_xilinx_domain/bsp/ps7_cortexa9_1/libsrc/libmetal_v2_5/build_libmetal/lib/include/metal/system/freertos/condition.h")
endif()

if(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include" TYPE FILE RENAME "metal/system/freertos/io.h" FILES "/home/excessive/github/controller-software/controller-software/vitis/osrc/ps7_cortexa9_1/freertos10_xilinx_domain/bsp/ps7_cortexa9_1/libsrc/libmetal_v2_5/build_libmetal/lib/include/metal/system/freertos/io.h")
endif()

if(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include" TYPE FILE RENAME "metal/system/freertos/irq.h" FILES "/home/excessive/github/controller-software/controller-software/vitis/osrc/ps7_cortexa9_1/freertos10_xilinx_domain/bsp/ps7_cortexa9_1/libsrc/libmetal_v2_5/build_libmetal/lib/include/metal/system/freertos/irq.h")
endif()

if(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include" TYPE FILE RENAME "metal/system/freertos/log.h" FILES "/home/excessive/github/controller-software/controller-software/vitis/osrc/ps7_cortexa9_1/freertos10_xilinx_domain/bsp/ps7_cortexa9_1/libsrc/libmetal_v2_5/build_libmetal/lib/include/metal/system/freertos/log.h")
endif()

if(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include" TYPE FILE RENAME "metal/system/freertos/mutex.h" FILES "/home/excessive/github/controller-software/controller-software/vitis/osrc/ps7_cortexa9_1/freertos10_xilinx_domain/bsp/ps7_cortexa9_1/libsrc/libmetal_v2_5/build_libmetal/lib/include/metal/system/freertos/mutex.h")
endif()

if(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include" TYPE FILE RENAME "metal/system/freertos/sleep.h" FILES "/home/excessive/github/controller-software/controller-software/vitis/osrc/ps7_cortexa9_1/freertos10_xilinx_domain/bsp/ps7_cortexa9_1/libsrc/libmetal_v2_5/build_libmetal/lib/include/metal/system/freertos/sleep.h")
endif()

if(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include" TYPE FILE RENAME "metal/system/freertos/sys.h" FILES "/home/excessive/github/controller-software/controller-software/vitis/osrc/ps7_cortexa9_1/freertos10_xilinx_domain/bsp/ps7_cortexa9_1/libsrc/libmetal_v2_5/build_libmetal/lib/include/metal/system/freertos/sys.h")
endif()

if(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include" TYPE FILE RENAME "metal/system/freertos/device.h" FILES "/home/excessive/github/controller-software/controller-software/vitis/osrc/ps7_cortexa9_1/freertos10_xilinx_domain/bsp/ps7_cortexa9_1/libsrc/libmetal_v2_5/build_libmetal/lib/include/metal/system/freertos/device.h")
endif()

if(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include" TYPE FILE RENAME "metal/system/freertos/zynq7/sys.h" FILES "/home/excessive/github/controller-software/controller-software/vitis/osrc/ps7_cortexa9_1/freertos10_xilinx_domain/bsp/ps7_cortexa9_1/libsrc/libmetal_v2_5/build_libmetal/lib/include/metal/system/freertos/zynq7/sys.h")
endif()

if(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include" TYPE FILE RENAME "metal/system/freertos/xlnx_common/sys.h" FILES "/home/excessive/github/controller-software/controller-software/vitis/osrc/ps7_cortexa9_1/freertos10_xilinx_domain/bsp/ps7_cortexa9_1/libsrc/libmetal_v2_5/build_libmetal/lib/include/metal/system/freertos/xlnx_common/sys.h")
endif()

if(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "/home/excessive/github/controller-software/controller-software/vitis/osrc/ps7_cortexa9_1/freertos10_xilinx_domain/bsp/ps7_cortexa9_1/libsrc/libmetal_v2_5/build_libmetal/lib/libmetal.a")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("/home/excessive/github/controller-software/controller-software/vitis/osrc/ps7_cortexa9_1/freertos10_xilinx_domain/bsp/ps7_cortexa9_1/libsrc/libmetal_v2_5/build_libmetal/lib/compiler/cmake_install.cmake")
  include("/home/excessive/github/controller-software/controller-software/vitis/osrc/ps7_cortexa9_1/freertos10_xilinx_domain/bsp/ps7_cortexa9_1/libsrc/libmetal_v2_5/build_libmetal/lib/processor/cmake_install.cmake")
  include("/home/excessive/github/controller-software/controller-software/vitis/osrc/ps7_cortexa9_1/freertos10_xilinx_domain/bsp/ps7_cortexa9_1/libsrc/libmetal_v2_5/build_libmetal/lib/system/cmake_install.cmake")

endif()

