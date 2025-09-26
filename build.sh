#!/bin/sh
clear
if [ ${#} = 0 ];then
     PLATFROM="x86"
else
    PLATFROM=$1
fi
echo "compile $PLATFROM"

export INSTALL_PATH=$(pwd)/3rdlib/$PLATFROM
mkdir -p ${INSTALL_PATH}
rm -rf build
mkdir build
tool_prefix=../${PLATFROM}.cmake
export PLATFORM="${PLATFROM}"
echo "config:  ${tool_prefix}---${PLATFORM}"


export AWTK_INCLUDE=$(pwd)/include/awtk/src
cmake -DCMAKE_TOOLCHAIN_FILE=${tool_prefix} -S src -B build -DPLATFORM=${PLATFORM} -DAWTK_INCLUDE=${AWTK_INCLUDE}

cmake --build build

echo cmake --install build --prefix ..
cmake --install build --prefix ${INSTALL_PATH}
