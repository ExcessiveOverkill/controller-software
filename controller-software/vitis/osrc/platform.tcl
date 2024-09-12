# 
# Usage: To re-create this platform project launch xsct with below options.
# xsct /home/excessive/github/controller-software/controller-software/vitis/osrc/platform.tcl
# 
# OR launch xsct and run below command.
# source /home/excessive/github/controller-software/controller-software/vitis/osrc/platform.tcl
# 
# To create the platform in a different location, modify the -out option of "platform create" command.
# -out option specifies the output directory of the platform project.

platform create -name {osrc}\
-hw {/home/excessive/github/controller-software/controller-firmware/Vivado/controller_firmware_top.xsa}\
-proc {ps7_cortexa9_1} -os {freertos10_xilinx} -no-boot-bsp -out {/home/excessive/github/controller-software/controller-software/vitis}

platform write
platform generate -domains 
platform active {osrc}
platform generate
bsp reload
bsp setlib -name openamp -ver 1.8
bsp config extra_compiler_flags "-mcpu=cortex-a9 -mfpu=vfpv3 -mfloat-abi=hard -nostartfiles -g -Wall -Wextra -fno-tree-loop-distribute-patterns -DUSE_AMP=1"
bsp write
bsp reload
catch {bsp regenerate}
platform generate -domains freertos10_xilinx_domain 
platform active {osrc}
