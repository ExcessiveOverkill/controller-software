install cross-platform compilers: sudo apt install gcc-arm-linux-gnueabihf g++-arm-linux-gnueabihf

install build tools: 
sudo apt update
sudo apt install build-essential ninja-build

install debugger:
sudo apt install gdb-multiarch

create ssh keys:
ssh-keygen -t rsa -b 4096 -C "your_email@example.com"
save them in the core/ssh folder

copy ssh key to zynq:
ssh-copy-id -i .ssh/zynq em-os@192.168.1.238

create ssh config:
mkdir -p ~/.ssh && chmod 700 ~/.ssh

create config:
touch ~/.ssh/config

set permissions:
chmod 600 ~/.ssh/config

configure ssh settings:
nano ~/.ssh/config

add config:
Host zynq
    HostName 192.168.1.238    
    User em-os
    IdentityFile /home/excessive/controller-software/controller-software/core/.ssh/zynq
    Port 22







petalinux setup:
package images:
petalinux-package --boot --fsbl --u-boot --force

create disk image:
petalinux-package --wic --outdir /home/excessive --wic-extra-args "-c xz" -b "BOOT.BIN,image.ub,boot.scr" --wks project-spec/configs/rootfs.wks





