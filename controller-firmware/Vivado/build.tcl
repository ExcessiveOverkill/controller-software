open_project    S:/Vivado/controller_firmware/controller_firmware.xpr

update_files -from_files S:/Vivado/autogen_sources/controller.v -to_files S:/Vivado/controller_firmware/controller_firmware.srcs/sources_1/imports/autogen_sources/controller.v -filesets [get_filesets *]

update_module_reference controller_firmware_Controller_0_0

set_param general.maxThreads 8

# update compile order (important if you’ve added/removed modules)
update_compile_order -fileset sources_1

# reset any prior runs (so you always start fresh)
reset_run synth_1
# launch synthesis
launch_runs synth_1 -jobs 8
wait_on_run synth_1

# now implementation
reset_run impl_1
launch_runs impl_1 -to_step write_bitstream -jobs 8
wait_on_run impl_1

# cleanly exit
exit
