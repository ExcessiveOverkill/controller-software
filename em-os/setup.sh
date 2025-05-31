#!/bin/bash
# Set the project root environment variable
export PROJECT_ROOT=$(pwd)

# Source the Petalinux settings script (adjust the path if needed)
source ~/petalinux/settings.sh

echo "Environment initialized. PROJECT_ROOT is set to $PROJECT_ROOT"
