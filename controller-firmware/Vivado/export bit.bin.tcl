set script_dir [file dirname [file normalize [info script]]]
set bit_input "$script_dir/controller_firmware/controller_firmware.runs/impl_1/controller_firmware_top.bit"
set bit_output "$script_dir/bitfile.bit"
set bif_file "$script_dir/convert.bif"

file copy -force $bit_input $bit_output

# Keep the BIF path absolute so bootgen works regardless of the current directory.
set bif_chan [open $bif_file w]
puts $bif_chan "the_ROM_image:"
puts $bif_chan "{"
puts $bif_chan "    $bit_output"
puts $bif_chan "}"
close $bif_chan

set old_pwd [pwd]
cd $script_dir
exec bootgen -w -arch zynq -image $bif_file -process_bitstream bin
cd $old_pwd