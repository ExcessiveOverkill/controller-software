set repo_dir [lindex $argv 0]

platform create -name {osrc}\
-hw [file join $repo_dir controller-firmware Vivado controller_firmware_top.xsa]\
-proc {ps7_cortexa9_1} -os {freertos10_xilinx} -no-boot-bsp -out [file join $repo_dir controller-software vitis]

platform write
platform generate -domains 
platform active {osrc}
platform generate


