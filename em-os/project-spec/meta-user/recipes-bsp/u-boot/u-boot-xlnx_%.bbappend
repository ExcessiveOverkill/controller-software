FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

SRC_URI:append = " file://platform-top.h file://bsp.cfg"
SRC_URI += "file://user_2024-10-27-00-48-00.cfg \
            file://user_2024-11-11-00-54-00.cfg \
            "

