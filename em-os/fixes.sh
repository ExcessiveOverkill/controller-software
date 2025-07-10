#!/bin/bash

# fix repo issued caused by renaming branches, run after build if it fails due to missing repos
sed -i 's/branch=master/branch=main/g' ${PROJECT_ROOT}/components/yocto/layers/poky/meta/recipes-extended/cracklib/cracklib_2.9.8.bb

sed -i 's/branch=master/branch=main/g' ${PROJECT_ROOT}/components/yocto/layers/poky/meta/recipes-extended/cracklib/cracklib_2.9.8.bb

sed -i 's/branch=master/branch=main/g' ${PROJECT_ROOT}/components/yocto/layers/poky/meta/recipes-support/bmap-tools/bmap-tools_git.bb
