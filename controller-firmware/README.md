# controller-firmware (FPGA HDL)

## Dev environment setup

### Requirements
- [Visual Studio Code](https://code.visualstudio.com/)
- [Python 3.12](https://www.python.org/downloads/release/python-3123/)
- [Vivado 2023.1](https://www.xilinx.com/support/download/index.html/content/xilinx/en/downloadNav/vivado-design-tools/2023-1.html)
- [Surfer](https://surfer-project.org/) (optional, used to view HDL sim trace files)


### Setup (windows/linux)
#### 1. Run the "setup project" task in VS Code (Terminal -> Run Task... -> setup project)
This will:
- Run subst to create a new drive leading to vivado files, this is needed to keep path lengths short (windows only)
- Create the python virtual environment
- Install required python packages
- Create the vivado project

## Creating new FPGA modules
These modules add user-configurable functionality to the controller

### Register mapping
All modules must include a register map object, this will:
- Define the module and provide any global settings the it's software driver may need
- Define all memory addresses available to the driver
- Data types, sizes, and packing if applicable for all memory
- Group offsets and alignement in the case of modules that are designed to handle multiple instances of something (ex: 1-32 encoders)

### Ports
All modules must follow the same port format and timing sequence for memory access


#### View [registers2.py](python/src/registers2.py) and [example_module.py](python/src/example_module.py) for more details


## Commiting changes
### Vivado (only needed for vivado project changes, *not amaranth changes*)
#### Run the "save vivado project" task in VS Code (Terminal -> Run Task... -> save vivado project)
This will save the vivado project to a .tcl file which is in the repo and can be committed
