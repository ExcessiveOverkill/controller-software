set script_dir [file dirname [file normalize [info script]]]

open_project "$script_dir/controller_firmware/controller_firmware.xpr"

update_files -from_files "$script_dir/autogen_sources/controller.v" -to_files "$script_dir/controller_firmware/controller_firmware.srcs/sources_1/imports/autogen_sources/controller.v" -filesets [get_filesets *]

# add controller.v if it isn't already in the project sources
if {[llength [get_files -quiet "*controller.v"]] == 0} {
    add_files -fileset sources_1 "$script_dir/autogen_sources/controller.v"
} else {
    import_files -force -fileset sources_1 "$script_dir/autogen_sources/controller.v"
}

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
