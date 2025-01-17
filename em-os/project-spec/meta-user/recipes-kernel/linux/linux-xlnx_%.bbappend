FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI:append = " file://bsp.cfg"
KERNEL_FEATURES:append = " bsp.cfg"
SRC_URI += "file://user_2024-11-05-04-03-00.cfg \
            file://user_2024-11-12-22-53-00.cfg \
            "

