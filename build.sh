#!/bin/sh
clear
if [ ${#} = 0 ];then
 echo "$@ select a platform t113/t507/cv181"
 exit 0
fi
echo "compile $1" 

export INSTALL_PATH=$(pwd)/3rdlib/$1
mkdir -p ${INSTALL_PATH}
rm -rf build
mkdir build
tool_prefix=../${1}.cmake
export PLATFORM="${1}"
echo "config:  ${tool_prefix}---${PLATFORM}"
    

export AWTK_INCLUDE=$(pwd)/include/awtk/src
cmake -DCMAKE_TOOLCHAIN_FILE=${tool_prefix} -S src -B build -DPLATFORM=${PLATFORM} -DAWTK_INCLUDE=${AWTK_INCLUDE}

cmake --build build

echo cmake --install build --prefix ..  
cmake --install build --prefix ${INSTALL_PATH}  
