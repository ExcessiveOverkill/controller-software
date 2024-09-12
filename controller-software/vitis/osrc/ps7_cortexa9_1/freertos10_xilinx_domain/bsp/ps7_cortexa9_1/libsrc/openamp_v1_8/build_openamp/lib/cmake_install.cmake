# Install script for directory: /home/excessive/github/controller-software/controller-software/vitis/osrc/ps7_cortexa9_1/freertos10_xilinx_domain/bsp/ps7_cortexa9_1/libsrc/openamp_v1_8/src/open-amp/lib

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
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "/home/excessive/github/controller-software/controller-software/vitis/osrc/ps7_cortexa9_1/freertos10_xilinx_domain/bsp/ps7_cortexa9_1/libsrc/openamp_v1_8/build_openamp/lib/libopen_amp.a")
endif()

if(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include" TYPE DIRECTORY FILES "/home/excessive/github/controller-software/controller-software/vitis/osrc/ps7_cortexa9_1/freertos10_xilinx_domain/bsp/ps7_cortexa9_1/libsrc/openamp_v1_8/src/open-amp/lib/include/openamp")
endif()

if(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include" TYPE DIRECTORY FILES "/home/excessive/github/controller-software/controller-software/vitis/osrc/ps7_cortexa9_1/freertos10_xilinx_domain/bsp/ps7_cortexa9_1/libsrc/openamp_v1_8/build_openamp/include/generated/openamp")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("/home/excessive/github/controller-software/controller-software/vitis/osrc/ps7_cortexa9_1/freertos10_xilinx_domain/bsp/ps7_cortexa9_1/libsrc/openamp_v1_8/build_openamp/lib/virtio/cmake_install.cmake")
  include("/home/excessive/github/controller-software/controller-software/vitis/osrc/ps7_cortexa9_1/freertos10_xilinx_domain/bsp/ps7_cortexa9_1/libsrc/openamp_v1_8/build_openamp/lib/rpmsg/cmake_install.cmake")
  include("/home/excessive/github/controller-software/controller-software/vitis/osrc/ps7_cortexa9_1/freertos10_xilinx_domain/bsp/ps7_cortexa9_1/libsrc/openamp_v1_8/build_openamp/lib/remoteproc/cmake_install.cmake")
  include("/home/excessive/github/controller-software/controller-software/vitis/osrc/ps7_cortexa9_1/freertos10_xilinx_domain/bsp/ps7_cortexa9_1/libsrc/openamp_v1_8/build_openamp/lib/proxy/cmake_install.cmake")

endif()

