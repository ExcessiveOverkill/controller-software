# Set the project name and file paths
set project_name "controller_firmware"
set script_dir [file dirname [file normalize [info script]]]
set project_path "$script_dir/controller_firmware"
set export_script_path "$script_dir/controller_firmware.tcl"

# Open the Vivado project
open_project $project_path/$project_name.xpr

# Export the project to a Tcl script
write_project_tcl -force $export_script_path

# Close the Vivado project
close_project