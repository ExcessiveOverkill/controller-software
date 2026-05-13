# Set the project name
set project_name "controller_firmware"

# Get project directory from command line argument, or use script directory as default
if {[llength $argv] > 0} {
    set project_dir [lindex $argv 0]
} else {
    set project_dir [file dirname [file normalize [info script]]]
}

set project_path "$project_dir/$project_name"
set export_script_path "$project_dir/controller_firmware.tcl"

# Open the Vivado project
open_project $project_path/$project_name.xpr

# Remove incremental synthesis checkpoint artifacts so the exported project
# does not keep depending on a generated DCP.
set dcp_files [get_files -quiet -of_objects [get_filesets utils_1] *controller_firmware_top.dcp]
if {$dcp_files ne ""} {
    remove_files $dcp_files
}

foreach run_name {synth_1 synth_1_copy_1 synth_1_copy_2 synth_1_copy_3 impl_1 impl_1_copy_1 impl_1_copy_2} {
    set run_obj [get_runs -quiet $run_name]
    if {$run_obj ne ""} {
        catch { set_property -name "incremental_checkpoint" -value "" -objects $run_obj }
        catch { set_property -name "auto_incremental_checkpoint" -value "0" -objects $run_obj }
        catch { set_property -name "auto_incremental_checkpoint.directory" -value "" -objects $run_obj }
    }
}

# Export the project to a Tcl script
write_project_tcl -force $export_script_path

# Normalize the exported Tcl so it stays portable across Windows and Linux.
set export_fd [open $export_script_path r]
set export_script [read $export_fd]
close $export_fd

set export_script [string map [list \
    "S:/Vivado/" "\${origin_dir}/" \
    "\${origin_dir}/controller_firmware/controller_firmware.srcs/sources_1/imports/autogen_sources/controller.v" "\${origin_dir}/autogen_sources/controller.v" \
    "\$origin_dir/controller_firmware/controller_firmware.srcs/sources_1/imports/autogen_sources/controller.v" "\${origin_dir}/autogen_sources/controller.v"
] $export_script]

set filtered_lines {}
foreach line [split $export_script "\n"] {
    if {[string match {set_property -name "auto_rqs.directory"*} $line]} {
        continue
    }
    if {[string match {set_property -name "auto_incremental_checkpoint.directory"*} $line]} {
        continue
    }
    lappend filtered_lines $line
}

set export_fd [open $export_script_path w]
puts $export_fd [join $filtered_lines "\n"]
close $export_fd

# Close the Vivado project
close_project