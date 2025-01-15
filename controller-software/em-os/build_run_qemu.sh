#!/bin/bash

# build
petalinux-build

# package binary files
petalinux-package --boot --fsbl --u-boot --force

# create image
petalinux-package --wic --outdir ./images/linux -b "BOOT.BIN,image.ub,boot.scr" --wks project-spec/configs/rootfs.wks

# run emulator
petalinux-boot --qemu --kernel

