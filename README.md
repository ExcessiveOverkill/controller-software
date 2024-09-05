# controller-software

Firmware for the motion controller. This project consists of multiple modules;
 - Motion planning
 - Inverse Kinematics
- Config management web app and backend API

## Initial project setup

### Requirements
- [Visual Studio Code](https://code.visualstudio.com/)
- [Python 3.12](https://www.python.org/downloads/release/python-3123/)
- [Vivado 2023.1](https://www.xilinx.com/support/download/index.html/content/xilinx/en/downloadNav/vivado-design-tools/2023-1.html)
- [Vitis 2023.1](https://www.xilinx.com/support/download/index.html/content/xilinx/en/downloadNav/vitis/2023-1.html)
- [Surfer](https://surfer-project.org/) (optional, used to view HDL sim trace files)


### Setup (windows)
#### 1. Run the "setup project" task in VS Code (Terminal -> Run Task... -> setup project)
This will:
- Run subst to create a new drive leading to vivado files, this is needed to keep path lengths short
- Create the python virtual environment
- Install required python packages
- Create the vivado project



## Commiting changes
#### 1. Run the "save vivado project" task in VS Code (Terminal -> Run Task... -> save vivado project)
This will:
- Save the vivado project to a .tcl file which is in the repo and can be committed