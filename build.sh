#!/bin/sh
clear
if [ ${#} = 0 ];then
 echo "$@ select a platform T113/T507"
 exit 0
fi
echo "compile $1" 

export INSTALL_PATH=$(pwd)/3rdlib/$1
mkdir -p ${INSTALL_PATH}
rm -rf build
mkdir build
tool_prefix=./t113.cmake
if [ "$1" = "T113" ];then
    tool_prefix=$(pwd)/t113.cmake
    export PLATFORM="T113"
else
    tool_prefix=$(pwd)/t507.cmake
    export PLATFORM="T507"
fi

export AWTK_INCLUDE=$(pwd)/include/awtk/src
cmake -DCMAKE_TOOLCHAIN_FILE=${tool_prefix} -S src -B build -DPLATFORM=${PLATFORM} -DAWTK_INCLUDE=${AWTK_INCLUDE}

cmake --build build

echo cmake --install build --prefix ..  
cmake --install build --prefix ${INSTALL_PATH}  

adb push 3rdlib/T113/banner_menu/libbanner_menu.so  /tmp