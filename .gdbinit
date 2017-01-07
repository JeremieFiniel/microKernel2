# Read in the correct ELF file, to get at the runtime symbols.
file vexpress-a9.elf

# Setup two initial breakpoints, one at the assembly entry point 
# and one at the C entry point.
br _entry
br kmain

# Now connect to Qemu on the Qemu's standard debug port
target remote localhost:1234

# Note: the previous command can be abbreviated by: 
#    (gdb) tar rem :1234
# when typed at the command line.


